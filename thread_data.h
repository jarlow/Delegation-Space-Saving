#ifndef THREAD_DATA_H
#define THREAD_DATA_H

#include <pthread.h>
#include <sys/time.h>
#include "barrier.h"
#include "relation.h"
#include "sketches.h"
#include "cm_benchmark.h"

typedef struct
{
    int tid;
    Sketch * theSketch;
    Relation * theData;
    #if FIXED_DURATION
    int elementsProcessed;
    #endif
    struct timeval start;
    struct timeval end;
    int startIndex;
    int endIndex;
    double returnData;
    int numQueries;
    int numInserts;
    Sketch ** sketchArray;
    Sketch * theGlobalSketch;
}threadDataStruct;

Sketch * globalSketch;
int numberOfThreads;
threadDataStruct * threadData;
int * threadIds;
pthread_t *threads;
pthread_attr_t attr;
barrier_t barrier_global;
barrier_t barrier_started;
volatile int startBenchmark = 0;

#endif 