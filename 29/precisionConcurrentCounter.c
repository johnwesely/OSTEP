/* implementation of a precise concurrent counter
** all threads have direct accesss to global counter
** precision of counter is achieved at trade off of efficiency
** due to only one thread at a time being able
** modify or even access counter 
** Homework problem 2 from chapter 29 of OSTEP */ 

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <assert.h>

typedef struct __counter_t {
    int             value;
    pthread_mutex_t lock;
} counter_t;

void init(counter_t* c) {
    c->value = 0;
    pthread_mutex_init(&c->lock, 0);
}

void increment(counter_t* c) {
    pthread_mutex_lock(&c->lock);
    c->value++;
    pthread_mutex_unlock(&c->lock);
}

void decrement(counter_t* c) {
    pthread_mutex_lock(&c->lock);
    c->value--;
    pthread_mutex_unlock(&c->lock);
}

int get(counter_t* c) {
    pthread_mutex_lock(&c->lock);
    int ret = c->value;
    pthread_mutex_unlock(&c->lock);
    return ret;
}

// threaded procedure for incrementing counter one million times
void* millionIncs(void* arg) {
    for (size_t i = 0; i < 1000000; ++i) {
        increment((counter_t*) arg);
    }
    return 0;
} 

int main(void) {
    counter_t* c = malloc(sizeof(counter_t));
    init(c);
    struct timeval init, fin;
    pthread_t threads[4]; 
    int rc;
    char* count[] = { "one thread", "two threads", "three threads", "four threads" };
    
    // loop runs millionIncs procedure on increasing amount of threads and measures performance 
    for (size_t i = 0; i < 4; ++i) {
        gettimeofday(&init, 0);
        for (size_t j = 0; j < i + 1; ++j) {
            rc = pthread_create(&threads[j], 0, millionIncs, c);
            assert(!rc);
        }
        for (size_t j = 0; j < i + 1; ++j) {
            rc = pthread_join(threads[j], 0);
            assert(!rc);
        }
        gettimeofday(&fin, 0);
        printf("time to increment counter one million times on %s: %ld\n", 
                count[i], fin.tv_usec - init.tv_usec);
    }
    
    return 1;
}
