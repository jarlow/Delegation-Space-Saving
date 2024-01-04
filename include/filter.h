#ifndef FILTER_H
#define FILTER_H

#include "sketches.h"
#include "thread_data.h"
#include "cm_benchmark.h"
#include <x86intrin.h>

extern int MAX_FILTER_UNIQUES;

#define MAX_FILTER_SLACK 5

void printFilter(FilterStruct * filter){
    int i=0;
    for (i=0;i<MAX_FILTER_UNIQUES;i++){
        printf("%d ", filter->filter_count[i]);
    }
    printf("\n");
}

int findMinIndex(FilterStruct* filter){
    int min = filter->filter_count[0];
    int index = 0;
    int i=1;
    for (i=1; i<MAX_FILTER_UNIQUES; i++){
        if (filter->filter_count[i] < min){
            index = i;
            min = filter->filter_count[i];
        }
    }
    return index;
}

//Checks if key is in the filter.
//Return: the index if key is in the filter, otherwise -1
static inline int queryFilterIndex16(uint32_t key, uint32_t * filterIndexes){   
    const __m128i s_item = _mm_set1_epi32(key);
    __m128i *filter = (__m128i *)filterIndexes;
    
    __m128i f_comp = _mm_cmpeq_epi32(s_item, filter[0]);
    __m128i s_comp = _mm_cmpeq_epi32(s_item, filter[1]);
    __m128i t_comp = _mm_cmpeq_epi32(s_item, filter[2]);
    __m128i r_comp = _mm_cmpeq_epi32(s_item, filter[3]);

    f_comp = _mm_packs_epi32(f_comp, s_comp);
    t_comp = _mm_packs_epi32(t_comp, r_comp);
    f_comp = _mm_packs_epi32(f_comp, t_comp);

    int found  = _mm_movemask_epi8(f_comp);

    if (found){
        return __builtin_ctz(found);
    }
    else{
        return -1;
    }
}

static inline int queryFilterIndex(uint32_t key, uint32_t * filterIndexes, int max){
    for (int i =0; i < max; i++ ){
        if (filterIndexes[i] == key){
            return i;
        }
    }
    return -1;
}

// Check if key is in the filter
// Returns its frequency in the filter if its found, otherwise 0
unsigned int queryFilter(unsigned int key, FilterStruct * filter){
    int index = queryFilterIndex16(key, filter->filter_id);
    if (index == -1) return 0;
    return filter->filter_count[index];
}
void insertFilterWithWriteBack(threadDataStruct * localThreadData, unsigned int key){
    
    FilterStruct * filter = &(localThreadData->Filter);
    int qRes = queryFilterIndex16(key,filter->filter_id);
    if (qRes == -1){
        // not in the filter but filter has space
        if (filter->filterCount < MAX_FILTER_UNIQUES){
            filter->filter_id[filter->filterCount] = key;
            filter->filter_count[filter->filterCount] = 1;
            filter->filterCount++;
        }
        // not in the filter and filter is full
        // evict the old minimum and place the new element there
        // Note: when evicting an item might have been pushed in the sketch already in the past
        // Note: might be better to first insert the new element, check its freq and then decide if you should evict
        else{
            int minIndex = findMinIndex(filter);
            //evict the old minimum
            if (filter->filter_count[minIndex] % MAX_FILTER_SLACK){
                insert(localThreadData, filter->filter_id[minIndex], filter->filter_count[minIndex] % MAX_FILTER_SLACK);
            }
            //place the new element there
            filter->filter_id[minIndex] = key;
            filter->filter_count[minIndex] = filter->filter_count[minIndex] - filter->filter_count[minIndex] % MAX_FILTER_SLACK +1;
        }
    }
    else{ 
        filter->filter_count[qRes]++;
        if (!(filter->filter_count[qRes] % MAX_FILTER_SLACK)){
            unsigned int new_key = (unsigned int )(filter->filter_id[qRes]);
            insert(localThreadData, key, MAX_FILTER_SLACK);
        }
    }
}

//NOTE: only works with the local copies version
static inline void insertFilterNoWriteBack(threadDataStruct * localThreadData, unsigned int key, unsigned int increment){

    FilterStruct * filter = &(localThreadData->Filter);
    
    int qRes = queryFilterIndex16(key,filter->filter_id);
    if (qRes == -1){
        // not in the filter but filter has space
        if (filter->filterCount < MAX_FILTER_UNIQUES){
            filter->filter_id[filter->filterCount] = key;
            filter->filter_count[filter->filterCount] = increment;
           // filter->filter_old_count[filter->filterCount] = 0;
            filter->filterCount++;
        }
        // not in the filter and filter is full
        // 1) insert the new item, 2) find the new frequency and 3) decide if you should evict or keep the old
        else{
            unsigned int estimate = (unsigned int) localThreadData->theSketch->Update_Sketch_and_Query(key, increment);
            int minIndex = findMinIndex(filter);
            if (estimate > filter->filter_count[minIndex]){
                //int drift = filter->filter_count[minIndex] - filter->filter_old_count[minIndex];
                //if (drift>0){
                    //evict the old minimum
                //    insert(localThreadData, filter->filter_id[minIndex], drift);
                //}
                //place the new element there
                filter->filter_id[minIndex] = key;
                filter->filter_count[minIndex] = estimate;
                //filter->filter_old_count[minIndex] = estimate;
            }
        }
    }
    else{
        // found the new item in the filter. Just increase the count but don't write back. 
        filter->filter_count[qRes] += increment;
    }
}

static inline int tryInsertInDelegatingFilter(FilterStruct * filter, unsigned int key){
    if (filter->filterCount == 16) return 0;

    int qRes = queryFilterIndex16(key,filter->filter_id);
    if (qRes == -1){
        // not in the filter but filter has space
        if (filter->filterCount < MAX_FILTER_UNIQUES){
            filter->filter_id[filter->filterCount] = key;
            filter->filter_count[filter->filterCount] = 1;
            filter->filterCount++;
        }
        /*if (filter->filterCount == MAX_FILTER_UNIQUES){
            filter->filterFull = 1;
        }*/
    }
    else{
        filter->filter_count[qRes]++;
    }
    return 1;
}

static inline int tryInsertInDelegatingFilterWithList(FilterStruct * filter, unsigned int key, threadDataStruct * owningThread){
    if (filter->filterCount == MAX_FILTER_UNIQUES) return 0;

    int qRes = queryFilterIndex(key,filter->filter_id,filter->filterCount);
    if (qRes == -1){
        // not in the filter but filter has space
        if (filter->filterCount < MAX_FILTER_UNIQUES){
            filter->filter_id[filter->filterCount] = key;
            filter->filter_count[filter->filterCount] = 1;
            filter->filterCount++;
        }
        //If i just fillled the filter, push into the queue of the owner
        if (filter->filterCount == MAX_FILTER_UNIQUES){
            push(filter, &(owningThread->listOfFullFilters));
        }
    }
    else{
        filter->filter_count[qRes]++;
    }
    return 1;
}

// Insert an element into a filter, filter is full if either max sum or max uniques are reached
static inline void InsertInDelegatingFilterWithListAndMaxSum(FilterStruct * filter, unsigned int key){
    int qRes = queryFilterIndex(key,filter->filter_id,filter->filterCount);
    //int qRes = queryFilterIndex16(key,filter->filter_id);
    if (qRes == -1){
		// not in the filter, insert it
	    filter->filter_id[filter->filterCount] = key;
		filter->filter_count[filter->filterCount] = 1;
		filter->filterCount++;
    }
    else{
        filter->filter_count[qRes]++;
    }
}


#endif
