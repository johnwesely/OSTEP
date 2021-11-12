#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include "mapreduce.h"

#define MAX 4 

typedef struct __kvp {
    char* key;
    char* val;
} kvp_t;

typedef struct __reducerArgs {
    char*         key;
    kvp_t*        partition;
    Getter        get_func;
    unsigned long partition_number;
} reducerArgs_t;

typedef struct __MR_RunArgs {
    int         argc;
    char**      argv;
    Mapper      map;
    int         num_mappers;
    Reducer     reduce;
    int         num_reducers;
    Partitioner partition;
} MR_RunArgs_t;

typedef int (*compar)(const void *, const void*);

typedef struct __sortArgs {
    kvp_t*      partition;
    size_t      partitionPtr;
    size_t      size;
    compar      cmp;
} sortArgs_t;

kvp_t** partitions;
kvp_t* buff[MAX];
size_t count = 0;
size_t mapPtr = 0;
size_t catchPtr = 0;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
size_t finishCatch = 0;


pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// wrapper for qsort thread function
void* qsortWrap(void* arg);
// compare function for sorting key value pairs
int kvpCompare(const void* a, const void* b);
// creates num_reducer threads to sorch each partition 
void sortPartitions(size_t* partitionPtr, int num_reducers);

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

unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}

void MR_Emit(char* key, char* value) {
    kvp_t* emit = malloc(sizeof(kvp_t));
    emit->key = key; emit->val = value;
    pthread_mutex_lock(&lock);
    while(count == MAX) {
        pthread_cond_wait(&empty, &lock);
    }
    buff[mapPtr] = emit;
    mapPtr = (mapPtr + 1) % MAX;
    ++count;
    pthread_cond_signal(&fill);
    pthread_mutex_unlock(&lock);
    return;
}

void* MR_Catch(void* arg) {
    unsigned long hash;
    MR_RunArgs_t* args = (MR_RunArgs_t*) arg;
    // !!! add support for variable sized partition pointer array
    size_t partitionPtr[64] = { 0 };
    while (!finishCatch) {
        pthread_mutex_lock(&lock);
        while (count == 0) {
            pthread_cond_wait(&fill, &lock);
            if (finishCatch) {
                sortPartitions(partitionPtr, args->num_reducers);
                return 0;
            }
        }
        kvp_t* tmp = buff[catchPtr];
        catchPtr = (catchPtr + 1) % MAX;
        --count;
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&lock);

        hash = MR_DefaultHashPartition(tmp->key, args->num_reducers);
        partitions[hash][partitionPtr[hash]].key = tmp->key;
        partitions[hash][partitionPtr[hash]].val = tmp->val;
        free(tmp);
        ++partitionPtr[hash];
    }

    sortPartitions(partitionPtr, args->num_reducers);
    return 0;
}

void sortPartitions(size_t* partitionPtr, int num_reducers) {
    pthread_t* sortThreads = malloc(sizeof(pthread_t) * num_reducers);
    int rc;

    for (size_t i = 0; i < num_reducers; ++i) {
        sortArgs_t sortArgs = { partitions[i], partitionPtr[i], sizeof(kvp_t), kvpCompare };
        rc = pthread_create(&sortThreads[i], 0, qsortWrap, &sortArgs);
        assert (!rc);
    }

    for (size_t i = 0; i < num_reducers; ++i) {
        rc = pthread_join(sortThreads[i], 0);
        assert (!rc);
    }
    
    for (size_t i = 0; i < num_reducers; ++i) {
        printf("partiton %zu:\n", i);
        for (size_t j = 0; j < partitionPtr[i]; ++j) {
            printf("%s\n", partitions[i][j].key);
        }
    }
}

void initPartitions(int n) {
    partitions = malloc(sizeof(kvp_t*) * n);
    for (size_t i; i < n; ++i) {
        // !!! add support for larger partition later
        partitions[i] = malloc(sizeof(kvp_t) * 1024);
    }
    return;
}

void MR_Run(int argc, char *argv[], 
	    Mapper map, int num_mappers, 
	    Reducer reduce, int num_reducers, 
	    Partitioner partition) {

    // initialize partition data structure 
    initPartitions(num_reducers);      

    pthread_t mapThreads[num_mappers];
    pthread_t redThreads[num_reducers];
    pthread_t emitCatcher;

    // argument pack to pass to Mapper Handler Threads
    MR_RunArgs_t args = { argc, argv, map, num_mappers,
                          reduce, num_reducers, partition };
    int rc;
    size_t i = 0; size_t j = 0;
    
    // begin catcher thread 
    rc = pthread_create(&emitCatcher, 0, MR_Catch, &args);
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

    return;
}

void wordCountMap(char *file_name) {
    FILE *fp = fopen(file_name, "r");
    assert(fp != NULL);

    char *line = NULL;
    size_t size = 0;
    
    while (getline(&line, &size, fp) != -1) {
        char* dummy = line;
        char* token = strtok(dummy, " \t\n\r");
        while (token) {
            MR_Emit(token, "1");
            token = strtok(0, " \t\n\r");
        }
    }
    free(line);
    fclose(fp);
}

void* qsortWrap(void* arg) {
    sortArgs_t* args = (sortArgs_t*) arg;
    qsort(args->partition, args->partitionPtr, args->size, args->cmp);
    return 0;
}

int kvpCompare(const void* a, const void* b) {
    kvp_t* x = (kvp_t*) a;
    kvp_t* y = (kvp_t*) b;

    return strcmp(x->key, y->key);
}

void wordCountReduce(char *key, Getter get_next, int partition_number) {
    int count = 0;
    char *value;
    while ((value = get_next(key, partition_number)) != NULL)
        count++;
    printf("%s %d\n", key, count);
}


int main(int argc, char* argv[]) {

    MR_Run(argc, argv, wordCountMap, 10, wordCountReduce, 10, MR_DefaultHashPartition);
    printf("dummyline\n");
}