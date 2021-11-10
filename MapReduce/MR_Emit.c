#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#define MAX 4 

typedef struct __kvp {
    char* key;
    char* val;
} kvp_t;

kvp_t partitions[4][16];
size_t partitionPtr[4] = { 0 };
kvp_t* buff[MAX];
size_t count = 0;
size_t mapPtr = 0;
size_t catchPtr = 0;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
size_t finishCatch = 0;


pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

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
    while (!finishCatch) {
        pthread_mutex_lock(&lock);
        while (count == 0) {
            pthread_cond_wait(&fill, &lock);
            if (finishCatch) return 0;
        }
        kvp_t* tmp = buff[catchPtr];
        catchPtr = (catchPtr + 1) % MAX;
        --count;
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&lock);

        hash = MR_DefaultHashPartition(tmp->key, 4);
        partitions[hash][partitionPtr[hash]].key = tmp->key;
        partitions[hash][partitionPtr[hash]].val = tmp->val;
        free(tmp);
        ++partitionPtr[hash];
    }
    return 0;
}

void* emitThread(void* arg) {
    kvp_t* arr = (kvp_t*) arg;
    for (size_t i = 0; i < 5; ++i) {
        MR_Emit(arr[i].key, arr[i].val);
    }
    return 0;
}

void testEmit(void) {
    kvp_t arrOne[] = { { "kee", "1" }, { "bee", "1" }, { "three", "1" }, { "pee", "1" }, { "kee", "1" } };
    kvp_t arrTwo[] = { { "boo", "1" }, { "throo", "1" }, { "poo", "1" }, { "boo", "1" }, { "koo", "1" } };
    kvp_t arrThr[] = { { "kaa", "1" }, { "baa", "1" }, { "thraa", "1" }, { "paa", "1" }, { "kaa", "1" } };
    kvp_t arrFou[] = { { "buu", "1" }, { "thruu", "1" }, { "puu", "1" }, { "buu", "1" }, { "kuu", "1" } };

    pthread_t threads[4];
    int rc;

    rc = pthread_create(&threads[0], 0, emitThread, arrOne); assert(!rc);
    rc = pthread_create(&threads[1], 0, emitThread, arrTwo); assert(!rc);
    rc = pthread_create(&threads[2], 0, emitThread, arrThr); assert(!rc);
    rc = pthread_create(&threads[3], 0, emitThread, arrFou); assert(!rc);

    rc = pthread_join(threads[0], 0); assert(!rc);
    rc = pthread_join(threads[1], 0); assert(!rc);
    rc = pthread_join(threads[2], 0); assert(!rc);
    rc = pthread_join(threads[3], 0); assert(!rc);
    ++finishCatch;

    pthread_cond_signal(&fill);

    return;
}

int main(void) {
    pthread_t catch;
    int rc;

    rc = pthread_create(&catch, 0, MR_Catch, 0); assert(!rc);
    testEmit();
    rc = pthread_join(catch, 0);
    rc = 42;
    return 1;
}
