#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include "mapreduce.h"

#define MAX_BUFF 16       // max size of buffer between emit and catch
#define PART_SIZE 1024    // max size of each partition 
#define MAX_PARTITIONS 64 // max number of partitions and by extension, reducer threads


// data structures for passing args to functions called by pthread create
typedef struct __reducer_args {
    Reducer      reducer_fn;
    unsigned long partition_number;
} reducer_args_t;

typedef struct __MR_run_args {
    int         argc;
    char**      argv;
    Mapper      map;
    int         num_mappers;
    Reducer     reduce;
    int         num_reducers;
    Partitioner partition;
} MR_run_args_t;

typedef struct __sort_args {
    kvp_t*      partition;
    size_t      partition_ptr;
    size_t      size;
    compar      cmp;
} sort_args_t;


kvp_t** partitions;
// globals for use between map and catch threads
kvp_t* buff[MAX_BUFF];
size_t count = 0;
size_t map_ptr = 0;
size_t catch_ptr = 0;
// conditions variables for interaction between map and catch threads
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
size_t finishCatch = 0;
Reducer red_fn;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Map Stage
void MR_emit(char* key, char* value);
void* MR_catch(void* arg);
// Reduce Stage
void* MR_reduce(void* args);
// initializes partitions to n arrays of PART_SIZE length
void init_partitions(int n);
// sorts each partition into alphabetical order 
void sort_partitions(size_t* partition_ptr, int num_reducers);
// compare function for sorting key value pairs
int kvp_compare(const void* a, const void* b);
// implementation fo strsep for use by user provided map function 
char* strsep(char **stringp, const char *delim);
// default hash function
// if desired, user can provide alternate of their choosing 
unsigned long MR_default_hash_partition(char *key, int num_partitions);


// top level function called by user to run map/reduce on supplied data
void MR_Run(int argc, char *argv[], 
	    Mapper map, int num_mappers, 
	    Reducer reduce, int num_reducers, 
	    Partitioner partition) {

    if (num_reducers > MAX_PARTITIONS) {
        printf("num_reducers must be less than %d\n", MAX_PARTITIONS);
        return;
    }

    if (argc == 1) {
        printf("user must supply files\n");
        return;
    }

    // initialize partition data structure 
    init_partitions(num_reducers);      

    red_fn = reduce;
    pthread_t mapThreads[num_mappers];
    pthread_t redThreads[num_reducers];
    pthread_t emitCatcher;

    // argument pack to pass to Mapper Handler Threads
    MR_run_args_t args = { argc, argv, map, num_mappers,
                          reduce, num_reducers, partition };
    int rc;
    size_t i = 0; size_t j = 0;
    
    // begin catcher thread 
    rc = pthread_create(&emitCatcher, 0, MR_catch, &args);
    assert(!rc);

    // create one mapper thread per file
    // up to num_mappers at a time 
    while (i < argc - 1) {
        for (j = 0; j < num_mappers && i < argc - 1; ++i, ++j) {
            rc = pthread_create(&mapThreads[j], 0, map, argv[i+1]);
            assert(!rc);
        }
        for (size_t k = 0; k < j; ++k) {
            rc = pthread_join(mapThreads[k], 0);
            assert(!rc);
        }
    }

    // wake up catcher if needed and signal to complete 
    ++finishCatch;
    pthread_mutex_lock(&lock);
    pthread_cond_signal(&fill);
    pthread_mutex_unlock(&lock);
    rc = pthread_join(emitCatcher, 0);
    assert(!rc);

    // reduce stage 
    // create one reducer thread per partition
    reducer_args_t redArgs[num_reducers];

    for (i = 0; i < num_reducers; ++i) {
        redArgs[i].reducer_fn = reduce;
        redArgs[i].partition_number = i;
        rc = pthread_create(&redThreads[i], 0, MR_reduce, &redArgs[i]);
        assert(!rc);
    }
    
    for (i = 0; i < num_reducers; ++i) {
        rc = pthread_join(redThreads[i], 0);
        assert(!rc);
    }

    // free memory alloced for partition array
    for (i = 0; i < num_reducers; ++i) {
        size_t part_index = 0;
        while (partitions[i][part_index].key) {
            free(partitions[i][part_index].key);
            free(partitions[i][part_index++].val);
        }
    }

    return;
}

// function called by user supplied map to pass data to catcher thread
void MR_emit(char* key, char* value) {
    kvp_t* emit = malloc(sizeof(kvp_t));
    emit->key = malloc(strlen(key)+1);
    emit->val = malloc(strlen(value)+1);
    strcpy(emit->key, key);
    strcpy(emit->val, value);
    pthread_mutex_lock(&lock);
    while(count == MAX_BUFF) {
        pthread_cond_wait(&empty, &lock);
    }
    buff[map_ptr] = emit;
    map_ptr = (map_ptr + 1) % MAX_BUFF;
    ++count;
    pthread_cond_signal(&fill);
    pthread_mutex_unlock(&lock);
    return;
}

// catcher thread receives data from emit, places data into appropriate partition,
// and then sorts partitions before returning 
void* MR_catch(void* arg) {
    unsigned long hash;
    MR_run_args_t* args = (MR_run_args_t*) arg;
    size_t partition_ptr[MAX_PARTITIONS] = { 0 };
    while (!finishCatch) {
        pthread_mutex_lock(&lock);
        while (count == 0) {
            pthread_cond_wait(&fill, &lock);
            if (finishCatch) {
                sort_partitions(partition_ptr, args->num_reducers);
                return 0;
            }
        }
        kvp_t* tmp = buff[catch_ptr];
        catch_ptr = (catch_ptr + 1) % MAX_BUFF;
        --count;
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&lock);

        hash = MR_default_hash_partition(tmp->key, args->num_reducers);
        partitions[hash][partition_ptr[hash]].key = tmp->key;
        partitions[hash][partition_ptr[hash]].val = tmp->val;
        ++partition_ptr[hash];
    }

    sort_partitions(partition_ptr, args->num_reducers);
    return 0;
}

// sorts each partition alphabetically with qsort
void sort_partitions(size_t* partition_ptr, int num_reducers) {
    for (size_t i = 0; i < num_reducers; ++i) {
        qsort(partitions[i], partition_ptr[i], sizeof(kvp_t), kvp_compare);
    }
} 

// handler function for calling user provided reduce function 
void* MR_reduce(void* args) {
    reducer_args_t* redArgs = (reducer_args_t*) args;

    char* key = partitions[redArgs->partition_number][0].key;
    red_fn(key, redArgs->partition_number);

    return 0;
}

// initializes partitions to n arrays of PART_SIZE length
void init_partitions(int n) {
    partitions = malloc(sizeof(kvp_t*) * n);
    for (size_t i = 0; i < n; ++i) {
        partitions[i] = malloc(sizeof(kvp_t) * PART_SIZE);
    }
    return;
}

// function for comparing key value pairs alphabetically 
int kvp_compare(const void* a, const void* b) {
    kvp_t* x = (kvp_t*) a;
    kvp_t* y = (kvp_t*) b;
    kvp_t  xx = *x;
    kvp_t  yy = *y;

    return strcmp(xx.key, yy.key);
}

char *strsep(char **stringp, const char *delim) {
    char *rv = *stringp;
    if (rv) {
        *stringp += strcspn(*stringp, delim);
        if (**stringp)
            *(*stringp)++ = '\0';
        else
            *stringp = 0; }
    return rv;
}

unsigned long MR_default_hash_partition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}
