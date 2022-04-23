#ifndef THREAD_DATA_H
#define THREAD_DATA_H

#include <pthread.h>
#include <sys/time.h>
#include "barrier.h"
#include "relation.h"
#include "sketches.h"
#include "libdivide.h"
#include <unordered_set>
#include "lossycount.h"
#include "LossyCountMinSketch.h"
#include "concurrentqueue.h"


//#define FILTER_SIZE 64
#define BUCKETS 512 //256//64
#define BUCKETS_SQ 262144 //65536//4096

typedef struct alignas(64) Filter_T{
    uint32_t *filter_id;
    uint32_t *filter_count;
    struct alignas(64){
        volatile int filterCount=0;
    };
        Filter_T * volatile next;
    struct alignas(64){
        volatile bool filterFull=false;
    };
}FilterStruct;

void push(FilterStruct* filter, FilterStruct* volatile*  headPointer){
    FilterStruct * volatile oldHead = *headPointer;
    filter->next = oldHead;
    while(! __sync_bool_compare_and_swap (headPointer, oldHead, filter)){
        oldHead = *headPointer;
        filter->next = oldHead;
    }
}

FilterStruct* pop(FilterStruct* volatile* headPointer){
    FilterStruct* volatile oldHead = *headPointer;
    FilterStruct* volatile newHead = oldHead->next;

    while(! __sync_bool_compare_and_swap (headPointer, oldHead, newHead)){
        oldHead = *headPointer;
        newHead = oldHead->next;
    }
    return oldHead;
}


typedef struct
{
    moodycamel::ConcurrentQueue<FilterStruct*>* concurrentQueue;
    LossySketch* th_local_sketch;
    Xi** randoms;
    Frequent_CM_Sketch* topkapi_instance;
    int* buckets; // for cardinality estimation;
    float sum_of_buckets=0;// for cardinality estimation;
    int* latencies;
    uint64_t numInsertedFilters=0;
    uint64_t accumFilters=0;
    vector<pair<uint32_t,uint32_t>> lasttopk; 
    LCL_type* ss; //Space-Saving instance
    std::unordered_set<int> * uniques;
    uint64_t num_uniques=0;
    int tid;
    Count_Min_Sketch * theSketch;
    Relation * theData;
    int elementsProcessed;
    struct timeval start;
    struct timeval end;
    unsigned long * seeds;
    int startIndex;
    int endIndex;
    struct libdivide::libdivide_s32_t * fastDivHandle;
    double returnData;
    FilterStruct Filter;
    FilterStruct * volatile listOfFullFilters;
    int * pendingQueriesKeys; // need volatiles?
    unsigned int * pendingQueriesCounts;
    volatile int * pendingQueriesFlags;
    volatile float * pendingTopKQueriesFlags;
    struct{
        volatile int insertsPending;
        char pad1[60];
    };
    struct{
        volatile int queriesPending;
        char pad2[60];
    };
    int numQueries;
    int numOps;
    int numTopKQueries;
    struct alignas(64){
        uint64_t substreamSize=0; // counter for each thread
        char pad3[56];
    };
    Count_Min_Sketch ** sketchArray;
    Count_Min_Sketch * theGlobalSketch;
    pthread_mutex_t mutex;
    uint16_t sumcounter=0;
}threadDataStruct;

Count_Min_Sketch * globalSketch;
int numberOfThreads;
threadDataStruct * threadData;
int * threadIds;
pthread_t *threads;
pthread_attr_t attr;
barrier_t barrier_global;
barrier_t barrier_started;
volatile int startBenchmark = 0;
volatile int startQueries = 0;

#endif 