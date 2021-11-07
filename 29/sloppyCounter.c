/* implementation of a sloppy concurrent counter
** threads have accesss to one counter per cpu
** global value is only updated when a threads counter 
** reaches a threshold defined by the user 
** efficiancy of counter is achieved at trade off of accuracy  
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <assert.h>

#define NUMCPUS 4

typedef struct __counter_t {
    int             global;
    pthread_mutex_t glock;
    int             local[NUMCPUS];
    pthread_mutex_t llock[NUMCPUS];
    int             threshold;
} counter_t;

typedef struct __args_t {
    counter_t* c;
    int        threadID;
} args_t;

void init(counter_t* c, int threshold) {
    c->threshold = threshold;
    c->global = 0;                           // initialize global counter
    pthread_mutex_init(&c->glock, 0);        // initalize global lock

    for (size_t i = 0; i < NUMCPUS; ++i) {
        c->local[i] = 0;                     //initialize local counters
        pthread_mutex_init(&c->llock[i], 0); //initalize local locks
    }
    return;
}

void update(counter_t* c, int threadID, int amt) {
    int cpu = threadID % NUMCPUS;
    pthread_mutex_lock(&c->llock[cpu]);      // lock local counter
    c->local[cpu] += amt;                    // increment local counter
    if (c->local[cpu] >= c->threshold) {     // if local counter exceeds threshold
        pthread_mutex_lock(&c->glock);
        c->global += c->local[cpu];          // increment global by local
        pthread_mutex_unlock(&c->glock);
        c->local[cpu] = 0;                   // reset local counter to 0
    }
    pthread_mutex_unlock(&c->llock[cpu]);
    return;
}

void increment(counter_t* c, int threadID) {
    update(c, threadID, 1);
}

void decrement(counter_t* c, int threadID) {
    update(c, threadID, -1);
}

// returns apporixmate total of global counter
int get(counter_t* c) {           
    pthread_mutex_lock(&c->glock);
    int ret = c->global;
    pthread_mutex_unlock(&c->glock);
    return ret;
}

// threaded procedure for incrementing counter one million times
void* millionIncs(void* arg) {
    args_t* args = (args_t*) arg;
    for (size_t i = 0; i < 10000000; ++i) {
        update(args->c, args->threadID, 1);
    }
    return 0;
} 

int main(void) {
    counter_t* c = malloc(sizeof(counter_t));
    init(c, 102400);
    struct timeval init, fin;
    pthread_t threads[4]; 
    size_t threadNum = 8;
    int rc;
    char* count[] = { "one thread", "two threads", "three threads", "four threads", "five threads",
                       "six threads", "seven threads", "eight threads" };
    
    // loop runs millionIncs procedure on increasing amount of threads and measures performance 
    for (size_t i = 0; i < threadNum; ++i) {
        gettimeofday(&init, 0);
        for (size_t j = 0; j < i + 1; ++j) {
            args_t args = { c, j };
            rc = pthread_create(&threads[j], 0, millionIncs, &args);
            assert(!rc);
        }
        for (size_t j = 0; j < i + 1; ++j) {
            rc = pthread_join(threads[j], 0);
            assert(!rc);
        }
        gettimeofday(&fin, 0);
        long double timeDiff = (fin.tv_sec - init.tv_sec) + ((fin.tv_usec - init.tv_usec) / 1000000);
        printf("time to increment counter ten million times on %s: %Lf\n", 
                count[i], timeDiff);
    }
    
    return 1;
}