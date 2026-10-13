#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
//the header of the dynamic array

#define DARRAY_MAGIC_FREED 0xDEADBEEF

typedef struct{
    uintptr_t canary;
    size_t count;
    size_t capacity;
    size_t elem_size;
    int (*compare_func)(void* elem1,void* elem2);
}DynArrayHeader;

#define darray_capacity(array) (((DynArrayHeader*)(array)-1)->capacity)
#define darray_is_empty(array) (darray_len(array) == 0)
#define darray_len(array) (((DynArrayHeader*)(array)-1)->count)

#define darray_reserve(array, reserved_count) \
    do { \
        size_t _needed = darray_len(array) + (reserved_count); \
        if (darray_capacity(array) < _needed) { \
            void* _tmp = darray_realloc((array), _needed); \
            if (_tmp) (array) = _tmp; \
        } \
    } while(0)
#define darray_shrink(array) \
    do { \
        void* _tmp = darray_realloc((array),(darray_len(array)>0?darray_len(array):8)); \
        if (_tmp) (array) = _tmp; \
    } while(0)

#define darray_push(array,value) \
    do{ \
        void* _tmp_darray=_darray_push_prep(array);\
        if(_tmp_darray) { \
            (array)=_tmp_darray;\
            (array)[darray_len(array)-1]=(value);\
        }\
    } while(0) 

#define darray_first(array) ((array)[0])
#define darray_last(array) ((array)[darray_len(array) > 0 ? darray_len(array) - 1 : 0])
#define darray_pop(array) ((array)[darray_len(array) > 0 ? --darray_len(array) : 0])
#define darray_clear(array) (((DynArrayHeader*)(array)-1)->count=0)
#define darray_insert(array,index,value) \
    do{ \
        size_t _in_idx = (index); \
        size_t _idx = _in_idx > darray_len(array) ? darray_len(array) : _in_idx; \
        void* tmp_darray=_darray_insert_prep(array,_idx);\
        if(tmp_darray) { \
            (array)=tmp_darray;\
            (array)[_idx]=(value);\
        }\
    } while(0) 

#define darray_contains(array, type, value) (darray_find((array), &(type){value}) != NULL)

#define darray_free(array) \
    do { \
        if (darray_is_valid(array)) { \
            ((DynArrayHeader*)(array)-1)->canary = DARRAY_MAGIC_FREED; \
            free(((DynArrayHeader*)(array)-1)); \
            (array) = NULL; \
        } \
    } while(0)
/*
macros that include the consume attribute are meant c stream type operations.
when we chain together many types of transforming functions ( both intermediate and terminal), the functions that allocate a new array may result 
in the original array being discarded and leaked. for that the consume attribute is used when we want to free the original array given to the transformation function.
an abstracted example could be as such:

arr=darray_init(4)
...
new_array=darray_filter(darray_map(arr,map_func),filter_func)
since we used the default filter function, the mapped array provided to the filter function was leaked after the filter function has finished executing.
to fix it, we would do the following:

new_array=darray_consume_filter(darray_map(arr,map_func),filter_func)
now since the comsume attribute is added(which is just a seperate macro), we signal to the filter function that the mapped array provided needs to be freed.
notice that the map function had remained the same, since it is the start of the stream, and we dont want to free the original array(arr). if you want to free the original array,
you could use the consume attribute for the map aswell.
*/ 

#define darray_map(array,map_func,mapped_elem_size) _darray_map(array,mapped_elem_size,map_func,false)
#define darray_consume_map(array,map_func,mapped_elem_size) _darray_map(array,mapped_elem_size,map_func,true)

#define darray_filter(array,filter_func) _darray_filter(array,filter_func,false)
#define darray_consume_filter(array,filter_func) _darray_filter(array,filter_func,true)

#define darray_concat_inplace(main_array, secondary_array) ((main_array) = _darray_merge((main_array), (secondary_array), false))
#define darray_consume_concat_inplace(main_array, secondary_array) ((main_array) = _darray_merge((main_array), (secondary_array), true))

#define darray_take(array,taken) _darray_take(array,taken,false)
#define darray_consume_take(array,taken) _darray_take(array,taken,true)

#define darray_skip(array,skipped) _darray_skip(array,skipped,false)
#define darray_consume_skip(array,skipped) _darray_skip(array,skipped,true)

#define darray_foreach(array, func) _darray_foreach(array, func, false)
#define darray_consume_foreach(array, func) _darray_foreach(array, func, true)

#define darray_any(array, func) _darray_any(array, func, false)
#define darray_consume_any(array, func) _darray_any(array, func, true)

#define darray_all(array, func) _darray_all(array, func, false)
#define darray_consume_all(array, func) _darray_all(array, func, true)

#define darray_reduce(array, accumulator, reduce_func) _darray_reduce(array,accumulator, reduce_func, false)
#define darray_consume_reduce(array, accumulator, reduce_func) _darray_reduce(array,accumulator, reduce_func, true)

#define darray_max(array) _darray_max(array,false)
#define darray_consume_max(array) _darray_max(array,true)

#define darray_min(array) _darray_min(array,false)
#define darray_consume_min(array) _darray_min(array,true)

#define darray_distinct(array) _darray_distinct(array,false)
#define darray_consume_distinct(array) _darray_distinct(array,true)

//checks whether a pointer is a valid dynamic array pointer.
bool darray_is_valid(void* array);
//initialize the dynamic array. if you wont need to use any comaprison on the array(sorting, finding, etc.), put NULL as the func
//returns NULL if elem_size is invalid or the allocation failed
void* darray_init(size_t initial_capacity,size_t elem_size,int (*compare_func)(void* elem1,void* elem2));
//reallocates the dynamic array.
//returns NULL if the array is an invalid dynamic array
void* darray_realloc(void* array,size_t new_capacity);

//removes an element from the array at index
void darray_remove(void* array,size_t index);

//sorts the given dynamic array using quick sort. the array is both changed and returned to allow for support of_consume operations
void* darray_sort(void* array);
//returns first instance of a specific elemnt in the array
void* darray_find(void* array,void* value);
//copies the array to a new array
void* darray_copy(void* array);
//reverses the array.the array is both changed and returned to allow for support of_consume operations
void* darray_reverse(void* array);
//provides a way to safetly set the compare function of the array
void* darray_set_compare(void* array, int (*compare_func)(void* elem1, void* elem2));

//maps the array and returns the new mapped array(original is freed if set by flag)
//note that since mapping can result in a different type of element, the comparison function of the new array becomes NULL
void* _darray_map(void* array,size_t mapped_elem_size,void (*map_func)(void* dest_elem,void* src_elem),bool free_array);
//filters the array by a given function and returns the new (original is freed if set by flag)
void* _darray_filter(void* array,bool (*filter_func)(void* elem),bool free_array);
// merges the secondary array at the end of the main array
void* _darray_merge(void* dest,void* src,bool free_array);
//returns the first n elements from the array 
void* _darray_take(void* array,size_t n,bool free_array);
//returns the array without the first n elements from the array 
void* _darray_skip(void* array,size_t n,bool free_array);
//executes a function for each element in the array
void _darray_foreach(void* array,void (*func)(void* elem),bool free_array);
// retuns whether all elements fulfill a condition func
bool _darray_all(void* array,bool (*condition_func)(void* elem),bool free_array);
// returns whether any element fulfills a condition func
bool _darray_any(void* array,bool (*condition_func)(void* elem),bool free_array);
// reduces the array into a single accumulator value
void _darray_reduce(void* array, void* accumulator, void (*reduce_func)(void* acc, void* elem), bool free_array);
//finds min in the array
void* _darray_min(void* array,bool free_array);
//finds max in the array
void* _darray_max(void* array,bool free_array);
// returns an array of all the distinct elements
void* _darray_distinct(void* array,bool free_array);

//internal allocation for insert function
void* _darray_insert_prep(void* array,size_t index);
//internal allocation for push function
void* _darray_push_prep(void* array);

#ifdef DYN_ARRAY_IMPLEMENTATION

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define CAST_HEADER(array) ((DynArrayHeader*)(array)-1)
#define CAST_ARRAY(header) (void*)((header)+1)

static uintptr_t g_darray_secret = 0;

static inline void init_canary_secret() {
    if (g_darray_secret == 0) {
        g_darray_secret = 0x7E3F1A9C4B2D8F51ULL ^ (uintptr_t)&g_darray_secret ^ (uintptr_t)time(NULL);
        if (g_darray_secret == 0) g_darray_secret = 1; 
    }
}

bool darray_is_valid(void* array) {
    if (!array || g_darray_secret == 0) return false;
    DynArrayHeader* header = CAST_HEADER(array);
    return (header->canary ^ (uintptr_t)header) == g_darray_secret;
}

static inline void* get_elem(void* array, size_t index, size_t elem_size) {
    return (char*)array + (index * elem_size);
}

void* darray_init(size_t initial_capacity,size_t elem_size,int (*compare_func)(void* elem1,void* elem2)){
    size_t actual_capacity = initial_capacity < 8 ? 8 : initial_capacity;
    if (!(elem_size > 0) || actual_capacity > (SIZE_MAX - sizeof(DynArrayHeader)) / elem_size) {
        fputs("Invalid Integer detected during array allocation.\n", stderr);
        return NULL;
    }
    DynArrayHeader* header=(DynArrayHeader*)malloc(actual_capacity*elem_size+sizeof(DynArrayHeader));
    if(!header){
        fputs("Couldn't allocate the dynamic array", stderr);
        return NULL;
    }
    init_canary_secret();
    header->canary = g_darray_secret ^ (uintptr_t)header;
    header->capacity=actual_capacity;
    header->count=0;
    header->compare_func=compare_func;
    header->elem_size=elem_size;
    return CAST_ARRAY(header);
}

void* darray_realloc(void* array,size_t new_capacity){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t elem_size=header->elem_size;
    if (new_capacity > (SIZE_MAX - sizeof(DynArrayHeader)) / elem_size) {
        fputs("Integer overflow detected during array reallocation.\n", stderr);
        return NULL;
    }
    DynArrayHeader* new_header = realloc(header, new_capacity * elem_size + sizeof(DynArrayHeader));
    if(!new_header){
        fputs("Couldn't re-allocate the dynamic array", stderr);
        return NULL;
    }
    new_header->capacity=new_capacity;
    if (new_header->count > new_capacity) {
        new_header->count = new_capacity;
    }
    new_header->canary = g_darray_secret ^ (uintptr_t)new_header;
    return CAST_ARRAY(new_header);
}
void* darray_copy(void* array) {
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header = CAST_HEADER(array);
    size_t elem_size=header->elem_size;
    void* new_array = darray_init(header->capacity, elem_size, header->compare_func);
    if (!new_array) {
        fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
        return NULL;
    }
    
    memcpy(new_array, array, header->count * elem_size);
    CAST_HEADER(new_array)->count = header->count;
    
    return new_array;
}

void* _darray_push_prep(void* array){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header = CAST_HEADER(array);
    if (header->count >= header->capacity) {
        size_t new_cap = header->capacity > SIZE_MAX / 2 ? SIZE_MAX : header->capacity * 2;
        void* new_array = darray_realloc(array, new_cap);
        if (!new_array){
            fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
            return NULL;
        }
        array = new_array;
        header = CAST_HEADER(array);
    }
    header->count++;
    return array;
}
void* _darray_insert_prep(void* array, size_t index){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header = CAST_HEADER(array);
    if (index>header->count) {
        index=header->count; 
    }
    if (header->count >= header->capacity) {
        size_t new_cap = header->capacity > SIZE_MAX / 2 ? SIZE_MAX : header->capacity * 2;
        void* new_array = darray_realloc(array, new_cap);
        if (!new_array) {
            fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
            return NULL;
        }
        
        array = new_array;
        header = CAST_HEADER(array);
    }
    if (index < header->count) {
        char* src = get_elem(array, index, header->elem_size);
        char* dest = get_elem(array, index+1, header->elem_size);
        size_t bytes_to_shift = (header->count-index)*header->elem_size;
        memmove(dest,src, bytes_to_shift);
    }
    header->count++;
    return array;
}

void darray_remove(void* array,size_t index){
    if (!darray_is_valid(array)) return;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t elem_size=header->elem_size;
    if(index>=header->count) return;
    char* src = get_elem(array, index+1, elem_size);
    char* dest = get_elem(array,index , elem_size);
    size_t bytes=(header->count-index-1) * elem_size;
    memmove(dest,src,bytes);
    header->count--;
}

void* _darray_map(void* array,size_t mapped_elem_size,void (*map_func)(void* dest_elem,void* src_elem),bool free_array){
    if (!array) return NULL;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t src_elem_size=header->elem_size;
    void* mapped_array=darray_init(header->count, mapped_elem_size, NULL);
    if(!mapped_array){
        fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
        if(free_array) darray_free(array);
        return NULL;
    }
    darray_len(mapped_array)=header->count;

    size_t len=header->count;
    for(size_t i=0;i<len;i++){
        map_func(get_elem(mapped_array, i,mapped_elem_size),get_elem(array, i,src_elem_size));
        
    }
    //if flag is set, free the original array(useful for chaining)
    if(free_array){
        darray_free(array);
    }
    return mapped_array;
}

void* _darray_filter(void* array,bool (*filter_func)(void* elem),bool free_array){
    if(!darray_is_valid(array)) return NULL;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t elem_size=header->elem_size;
    void* filtered_array=darray_init(header->count, elem_size, header->compare_func);
    if(!filtered_array){
        fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
        if(free_array) darray_free(array);
        return NULL;
    }
    size_t len=header->count,count=0;
    for(size_t i=0;i<len;i++){
        if(filter_func(get_elem(array, i, elem_size))){
            memcpy(get_elem(filtered_array, count, elem_size),get_elem(array, i, elem_size),elem_size);
            count++;
        }
    }
    darray_len(filtered_array)=count;
    if(free_array){
        darray_free(array);
    }
    return filtered_array;
}

static inline void swap(void* a, void* b, void* temp, size_t size){
    memcpy(temp, a, size);
    memcpy(a, b, size);
    memcpy(b, temp, size);
}

//partitions the array around thee element at index start(start and end must be valid indexes)
size_t qsort_partition(void* array, size_t start, size_t end, size_t elem_size, void* temp){
    char* pivot_elem = get_elem(array, end, elem_size);
    size_t i = start;
    for(size_t j = start; j < end; j++){
        if (CAST_HEADER(array)->compare_func(get_elem(array, j, elem_size), pivot_elem) < 0) {
            swap(get_elem(array, i, elem_size), get_elem(array, j, elem_size), temp, elem_size);
            i++;
        }
    }
    swap(get_elem(array, i, elem_size), get_elem(array, end, elem_size), temp, elem_size);
    return i;
}
void qsort_array(void* array,size_t start,size_t end,size_t elem_size,void* temp){
    if(start>=end) return;

    
    size_t pivot=qsort_partition(array, start, end, elem_size,temp);
    if (pivot > 0) {
        qsort_array(array, start, pivot - 1, elem_size,temp);
    }
    if (pivot < end) {
        qsort_array(array, pivot + 1, end, elem_size,temp);
    }
}
void* darray_sort(void* array){
    if(!darray_is_valid(array) || CAST_HEADER(array)->compare_func==NULL || CAST_HEADER(array)->count == 0) return array;
    size_t elem_size=CAST_HEADER(array)->elem_size;
    void* temp = malloc(elem_size);
    if (!temp) {
        fputs("Failed to allocate temp for sorting.\n", stderr);
        return array; 
    }
    qsort_array(array, 0, CAST_HEADER(array)->count-1, elem_size,temp);
    free(temp);
    return array;
}

void* darray_find(void* array,void* value){
    if(!darray_is_valid(array) || !value || CAST_HEADER(array)->compare_func==NULL) return NULL;
    size_t len=darray_len(array);
    size_t elem_size=CAST_HEADER(array)->elem_size;
    for(size_t i=0;i<len;i++){
        void* curr=get_elem(array, i, elem_size);
        if(CAST_HEADER(array)->compare_func(curr,value)==0) {
            return curr;
        }
    }
    return NULL;
}

void _darray_foreach(void* array,void (*func)(void* elem),bool free_array){
    if(!darray_is_valid(array)) return;
    size_t len=darray_len(array);
    size_t elem_size=CAST_HEADER(array)->elem_size;
    for(size_t i=0;i<len;i++){
        func(get_elem(array, i, elem_size));
    }
    if(free_array){
        darray_free(array);
    }
}

bool _darray_all(void* array,bool (*condition_func)(void* elem),bool free_array){
    if (!darray_is_valid(array)) return false;
    size_t len=darray_len(array);
    size_t elem_size=CAST_HEADER(array)->elem_size;
    for(size_t i=0;i<len;i++){
        if(!condition_func(get_elem(array, i,elem_size))){
            if(free_array) darray_free(array);
            return false;
        }
    }
    if(free_array){
        darray_free(array);
    }
    return true;
}

bool _darray_any(void* array,bool (*condition_func)(void* elem),bool free_array){
    if (!darray_is_valid(array)) return false;
    size_t len=darray_len(array);
    size_t elem_size=CAST_HEADER(array)->elem_size;
    for(size_t i=0;i<len;i++){
        if(condition_func(get_elem(array, i,elem_size))) {    
            if(free_array) darray_free(array);
            return true;
        }

    }
    if(free_array){
        darray_free(array);
    }
    return false;
}

void* _darray_merge(void* dest,void* src,bool free_array){
    if (!darray_is_valid(dest)) {
        if(darray_is_valid(src) && free_array) darray_free(src);
        return NULL;
    }
    if (!darray_is_valid(src)) return dest;

    bool is_self = (dest == src);
    DynArrayHeader* src_h=CAST_HEADER(src);
    DynArrayHeader* dest_h=CAST_HEADER(dest);
    size_t elem_size=CAST_HEADER(src)->elem_size;
    if (dest_h->elem_size != src_h->elem_size) {
        if(free_array && !is_self) darray_free(src);
        return dest; 
    }
    size_t original_src_count = src_h->count;

    if(original_src_count == 0 ) {
        if(free_array && !is_self) darray_free(src);
        return dest;
    }
    
    if (SIZE_MAX - dest_h->count < original_src_count) {
        fprintf(stderr, "Integer overflow detected in merge capacity calculation.\n");
        if(free_array && !is_self) darray_free(src);
        return dest;
    }
    size_t needed_capacity=src_h->count+dest_h->count;
    if(needed_capacity>dest_h->capacity){
        size_t new_cap=src_h->capacity>dest_h->capacity?src_h->capacity:dest_h->capacity;
        new_cap=new_cap>SIZE_MAX/2?SIZE_MAX:new_cap*2;
        void* tmp=darray_realloc(dest,new_cap);
        if(!tmp){
            fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
            if(free_array) darray_free(src);
            return dest;
        }
        dest=tmp;
        dest_h=CAST_HEADER(tmp);
        if (is_self) {
            src = tmp;
            src_h = CAST_HEADER(tmp);
        }
    }
    memcpy(get_elem(dest,dest_h->count,elem_size),src,src_h->count*elem_size);
    dest_h->count+=original_src_count;
    if(free_array && !is_self){
        darray_free(src);
    }
    return dest;
}

void* _darray_take(void* array,size_t n,bool free_array){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t elem_size=CAST_HEADER(array)->elem_size;
    if(n>header->count){
        n=header->count;
    }
    void* new_array=darray_init(n, elem_size, header->compare_func);
    if(!new_array){
        fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
        if(free_array) darray_free(array);
        return NULL;
    }
    memcpy(new_array, array, n*elem_size);
    CAST_HEADER(new_array)->count=n;
    if(free_array){
        darray_free(array);
    }
    return new_array;
}

void* _darray_skip(void* array,size_t n,bool free_array){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t elem_size=CAST_HEADER(array)->elem_size;
    if(n>header->count){
        n=header->count;
    }
    void* new_array=darray_init(header->count-n, elem_size, header->compare_func);
    if(!new_array){
        fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
        if(free_array) darray_free(array);
        return NULL;
    }
    memcpy(new_array, get_elem(array, n,elem_size), (header->count-n)*elem_size);
    CAST_HEADER(new_array)->count=header->count-n;
    if(free_array){
        darray_free(array);
    }
    return new_array;
}

void _darray_reduce(void* array, void* accumulator, void (*reduce_func)(void* acc, void* elem),bool free_array){
    if (!darray_is_valid(array)) return;
    if (!accumulator) {
        if (free_array) darray_free(array);
        return;
    }
    size_t len=darray_len(array);
    size_t elem_size=CAST_HEADER(array)->elem_size;
    for(size_t i=0;i<len;i++){
        reduce_func(accumulator,get_elem(array, i, elem_size));
    }
    if(free_array){
        darray_free(array);
    }
}

void* darray_reverse(void* array){
    if (!darray_is_valid(array) || darray_len(array) == 0) return array;
    size_t l=0,r=darray_len(array)-1;
    size_t elem_size=CAST_HEADER(array)->elem_size;
    void* temp = malloc(elem_size);
    if (!temp) {
        fputs("darray_reverse(): Failed to allocate temp for reversing.\n", stderr);
        return array; 
    }
    while(l<r){
        swap(get_elem(array, l, elem_size), get_elem(array, r, elem_size),temp, elem_size);
        l++;
        r--;
    }
    free(temp);
    return array;
}

void* _darray_min(void* array,bool free_array){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t elem_size=header->elem_size;
    if(header->count == 0 || header->compare_func == NULL) {
        if(free_array) darray_free(array);
        return NULL;
    }
    size_t len=darray_len(array);
    void* min=get_elem(array, 0, elem_size);
    for(size_t i=1;i<len;i++){
        if(header->compare_func(min,get_elem(array,i,elem_size))>0){
            min=get_elem(array,i,elem_size);
        }
    }
    if(free_array){
        void* safe_min = malloc(elem_size);
        if (safe_min) memcpy(safe_min, min, elem_size);
        darray_free(array);
        return safe_min; //caller is responsible for freeing this
    }
    return min;
}
void* _darray_max(void* array,bool free_array){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t elem_size=header->elem_size;
    if(header->count==0 || header->compare_func==NULL ) {
        if(free_array) darray_free(array);
        return NULL;
    }
    size_t len=darray_len(array);
    void* max=get_elem(array, 0, elem_size);
    for(size_t i=1;i<len;i++){
        if(header->compare_func(max,get_elem(array,i,elem_size))<0){
            max=get_elem(array,i,elem_size);
        }
    }
    if(free_array){
        void* safe_max = malloc(elem_size);
        if (safe_max) memcpy(safe_max, max, elem_size);
        darray_free(array);
        return safe_max; //caller is responsible for freeing this
    }
    return max;
}

void* _darray_distinct(void* array,bool free_array){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header = CAST_HEADER(array);
    size_t elem_size=header->elem_size;
    if (!header->compare_func) {
        if(free_array) darray_free(array);
        return NULL;
    }

    if (header->count <= 1) {
        // we pass it along instead of freeing it
        if(free_array) return array;
        void* ret_array= darray_copy(array);
        if(!ret_array){
            fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
            if(free_array) darray_free(array);
            return NULL;
        }
        return ret_array;
    }

    void* distinct_array = darray_copy(array);
    if(!distinct_array){
        fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
        if(free_array) darray_free(array);
        return NULL;
    }
    darray_sort(distinct_array);
    
    size_t write_idx = 1; // the next place to write a unique element
    size_t len = CAST_HEADER(distinct_array)->count;

    for (size_t read_idx = 1; read_idx < len; read_idx++) {
        void* prev = get_elem(distinct_array, write_idx - 1, elem_size);
        void* curr = get_elem(distinct_array, read_idx, elem_size);
        
        if (header->compare_func(prev, curr) != 0) {
            if (write_idx != read_idx) {
                memcpy(get_elem(distinct_array, write_idx, elem_size), curr, elem_size);
            }
            write_idx++;
        }
    }

    CAST_HEADER(distinct_array)->count = write_idx;

    if (free_array) darray_free(array);
    return distinct_array;
}

void* darray_set_compare(void* array, int (*compare_func)(void* elem1, void* elem2)){
    if (!darray_is_valid(array)) return NULL;
    
    DynArrayHeader* header = CAST_HEADER(array);
    header->compare_func = compare_func;
    
    return array; 
}

#endif // DYN_ARRAY_IMPLEMENTATION
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define CAST_HEADER(array) ((DynArrayHeader*)(array)-1)
#define CAST_ARRAY(header) (void*)((header)+1)

static uintptr_t g_darray_secret = 0;

static inline void init_canary_secret() {
    if (g_darray_secret == 0) {
        g_darray_secret = 0x7E3F1A9C4B2D8F51ULL ^ (uintptr_t)&g_darray_secret ^ (uintptr_t)time(NULL);
        if (g_darray_secret == 0) g_darray_secret = 1; 
    }
}

bool darray_is_valid(void* array) {
    if (!array || g_darray_secret == 0) return false;
    DynArrayHeader* header = CAST_HEADER(array);
    return (header->canary ^ (uintptr_t)header) == g_darray_secret;
}

static inline void* get_elem(void* array, size_t index, size_t elem_size) {
    return (char*)array + (index * elem_size);
}

void* darray_init(size_t initial_capacity,size_t elem_size,int (*compare_func)(void* elem1,void* elem2)){
    size_t actual_capacity = initial_capacity < 8 ? 8 : initial_capacity;
    if (!(elem_size > 0) || actual_capacity > (SIZE_MAX - sizeof(DynArrayHeader)) / elem_size) {
        fputs("Invalid Integer detected during array allocation.\n", stderr);
        return NULL;
    }
    DynArrayHeader* header=(DynArrayHeader*)malloc(actual_capacity*elem_size+sizeof(DynArrayHeader));
    if(!header){
        fputs("Couldn't allocate the dynamic array", stderr);
        return NULL;
    }
    init_canary_secret();
    header->canary = g_darray_secret ^ (uintptr_t)header;
    header->capacity=actual_capacity;
    header->count=0;
    header->compare_func=compare_func;
    header->elem_size=elem_size;
    return CAST_ARRAY(header);
}

void* darray_realloc(void* array,size_t new_capacity){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t elem_size=header->elem_size;
    if (new_capacity > (SIZE_MAX - sizeof(DynArrayHeader)) / elem_size) {
        fputs("Integer overflow detected during array reallocation.\n", stderr);
        return NULL;
    }
    DynArrayHeader* new_header = realloc(header, new_capacity * elem_size + sizeof(DynArrayHeader));
    if(!new_header){
        fputs("Couldn't re-allocate the dynamic array", stderr);
        return NULL;
    }
    new_header->capacity=new_capacity;
    if (new_header->count > new_capacity) {
        new_header->count = new_capacity;
    }
    new_header->canary = g_darray_secret ^ (uintptr_t)new_header;
    return CAST_ARRAY(new_header);
}
void* darray_copy(void* array) {
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header = CAST_HEADER(array);
    size_t elem_size=header->elem_size;
    void* new_array = darray_init(header->capacity, elem_size, header->compare_func);
    if (!new_array) {
        fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
        return NULL;
    }
    
    memcpy(new_array, array, header->count * elem_size);
    CAST_HEADER(new_array)->count = header->count;
    
    return new_array;
}

void* _darray_push_prep(void* array){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header = CAST_HEADER(array);
    if (header->count >= header->capacity) {
        size_t new_cap = header->capacity > SIZE_MAX / 2 ? SIZE_MAX : header->capacity * 2;
        void* new_array = darray_realloc(array, new_cap);
        if (!new_array){
            fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
            return NULL;
        }
        array = new_array;
        header = CAST_HEADER(array);
    }
    header->count++;
    return array;
}
void* _darray_insert_prep(void* array, size_t index){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header = CAST_HEADER(array);
    if (index>header->count) {
        index=header->count; 
    }
    if (header->count >= header->capacity) {
        size_t new_cap = header->capacity > SIZE_MAX / 2 ? SIZE_MAX : header->capacity * 2;
        void* new_array = darray_realloc(array, new_cap);
        if (!new_array) {
            fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
            return NULL;
        }
        
        array = new_array;
        header = CAST_HEADER(array);
    }
    if (index < header->count) {
        char* src = get_elem(array, index, header->elem_size);
        char* dest = get_elem(array, index+1, header->elem_size);
        size_t bytes_to_shift = (header->count-index)*header->elem_size;
        memmove(dest,src, bytes_to_shift);
    }
    header->count++;
    return array;
}

void darray_remove(void* array,size_t index){
    if (!darray_is_valid(array)) return;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t elem_size=header->elem_size;
    if(index>=header->count) return;
    char* src = get_elem(array, index+1, elem_size);
    char* dest = get_elem(array,index , elem_size);
    size_t bytes=(header->count-index-1) * elem_size;
    memmove(dest,src,bytes);
    header->count--;
}

void* _darray_map(void* array,size_t mapped_elem_size,void (*map_func)(void* dest_elem,void* src_elem),bool free_array){
    if (!array) return NULL;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t src_elem_size=header->elem_size;
    void* mapped_array=darray_init(header->count, mapped_elem_size, NULL);
    if(!mapped_array){
        fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
        if(free_array) darray_free(array);
        return NULL;
    }
    darray_len(mapped_array)=header->count;

    size_t len=header->count;
    for(size_t i=0;i<len;i++){
        map_func(get_elem(mapped_array, i,mapped_elem_size),get_elem(array, i,src_elem_size));
        
    }
    //if flag is set, free the original array(useful for chaining)
    if(free_array){
        darray_free(array);
    }
    return mapped_array;
}

void* _darray_filter(void* array,bool (*filter_func)(void* elem),bool free_array){
    if(!darray_is_valid(array)) return NULL;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t elem_size=header->elem_size;
    void* filtered_array=darray_init(header->count, elem_size, header->compare_func);
    if(!filtered_array){
        fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
        if(free_array) darray_free(array);
        return NULL;
    }
    size_t len=header->count,count=0;
    for(size_t i=0;i<len;i++){
        if(filter_func(get_elem(array, i, elem_size))){
            memcpy(get_elem(filtered_array, count, elem_size),get_elem(array, i, elem_size),elem_size);
            count++;
        }
    }
    darray_len(filtered_array)=count;
    if(free_array){
        darray_free(array);
    }
    return filtered_array;
}

static inline void swap(void* a, void* b, void* temp, size_t size){
    memcpy(temp, a, size);
    memcpy(a, b, size);
    memcpy(b, temp, size);
}

//partitions the array around thee element at index start(start and end must be valid indexes)
size_t qsort_partition(void* array, size_t start, size_t end, size_t elem_size, void* temp){
    char* pivot_elem = get_elem(array, end, elem_size);
    size_t i = start;
    for(size_t j = start; j < end; j++){
        if (CAST_HEADER(array)->compare_func(get_elem(array, j, elem_size), pivot_elem) < 0) {
            swap(get_elem(array, i, elem_size), get_elem(array, j, elem_size), temp, elem_size);
            i++;
        }
    }
    swap(get_elem(array, i, elem_size), get_elem(array, end, elem_size), temp, elem_size);
    return i;
}
void qsort_array(void* array,size_t start,size_t end,size_t elem_size,void* temp){
    if(start>=end) return;

    
    size_t pivot=qsort_partition(array, start, end, elem_size,temp);
    if (pivot > 0) {
        qsort_array(array, start, pivot - 1, elem_size,temp);
    }
    if (pivot < end) {
        qsort_array(array, pivot + 1, end, elem_size,temp);
    }
}
void* darray_sort(void* array){
    if(!darray_is_valid(array) || CAST_HEADER(array)->compare_func==NULL || CAST_HEADER(array)->count == 0) return array;
    size_t elem_size=CAST_HEADER(array)->elem_size;
    void* temp = malloc(elem_size);
    if (!temp) {
        fputs("Failed to allocate temp for sorting.\n", stderr);
        return array; 
    }
    qsort_array(array, 0, CAST_HEADER(array)->count-1, elem_size,temp);
    free(temp);
    return array;
}

void* darray_find(void* array,void* value){
    if(!darray_is_valid(array) || !value || CAST_HEADER(array)->compare_func==NULL) return NULL;
    size_t len=darray_len(array);
    size_t elem_size=CAST_HEADER(array)->elem_size;
    for(size_t i=0;i<len;i++){
        void* curr=get_elem(array, i, elem_size);
        if(CAST_HEADER(array)->compare_func(curr,value)==0) {
            return curr;
        }
    }
    return NULL;
}

void _darray_foreach(void* array,void (*func)(void* elem),bool free_array){
    if(!darray_is_valid(array)) return;
    size_t len=darray_len(array);
    size_t elem_size=CAST_HEADER(array)->elem_size;
    for(size_t i=0;i<len;i++){
        func(get_elem(array, i, elem_size));
    }
    if(free_array){
        darray_free(array);
    }
}

bool _darray_all(void* array,bool (*condition_func)(void* elem),bool free_array){
    if (!darray_is_valid(array)) return false;
    size_t len=darray_len(array);
    size_t elem_size=CAST_HEADER(array)->elem_size;
    for(size_t i=0;i<len;i++){
        if(!condition_func(get_elem(array, i,elem_size))){
            if(free_array) darray_free(array);
            return false;
        }
    }
    if(free_array){
        darray_free(array);
    }
    return true;
}

bool _darray_any(void* array,bool (*condition_func)(void* elem),bool free_array){
    if (!darray_is_valid(array)) return false;
    size_t len=darray_len(array);
    size_t elem_size=CAST_HEADER(array)->elem_size;
    for(size_t i=0;i<len;i++){
        if(condition_func(get_elem(array, i,elem_size))) {    
            if(free_array) darray_free(array);
            return true;
        }

    }
    if(free_array){
        darray_free(array);
    }
    return false;
}

void* _darray_merge(void* dest,void* src,bool free_array){
    if (!darray_is_valid(dest)) {
        if(darray_is_valid(src) && free_array) darray_free(src);
        return NULL;
    }
    if (!darray_is_valid(src)) return dest;

    bool is_self = (dest == src);
    DynArrayHeader* src_h=CAST_HEADER(src);
    DynArrayHeader* dest_h=CAST_HEADER(dest);
    size_t elem_size=CAST_HEADER(src)->elem_size;
    if (dest_h->elem_size != src_h->elem_size) {
        if(free_array && !is_self) darray_free(src);
        return dest; 
    }
    size_t original_src_count = src_h->count;

    if(original_src_count == 0 ) {
        if(free_array && !is_self) darray_free(src);
        return dest;
    }
    
    if (SIZE_MAX - dest_h->count < original_src_count) {
        fprintf(stderr, "Integer overflow detected in merge capacity calculation.\n");
        if(free_array && !is_self) darray_free(src);
        return dest;
    }
    size_t needed_capacity=src_h->count+dest_h->count;
    if(needed_capacity>dest_h->capacity){
        size_t new_cap=src_h->capacity>dest_h->capacity?src_h->capacity:dest_h->capacity;
        new_cap=new_cap>SIZE_MAX/2?SIZE_MAX:new_cap*2;
        void* tmp=darray_realloc(dest,new_cap);
        if(!tmp){
            fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
            if(free_array) darray_free(src);
            return dest;
        }
        dest=tmp;
        dest_h=CAST_HEADER(tmp);
        if (is_self) {
            src = tmp;
            src_h = CAST_HEADER(tmp);
        }
    }
    memcpy(get_elem(dest,dest_h->count,elem_size),src,src_h->count*elem_size);
    dest_h->count+=original_src_count;
    if(free_array && !is_self){
        darray_free(src);
    }
    return dest;
}

void* _darray_take(void* array,size_t n,bool free_array){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t elem_size=CAST_HEADER(array)->elem_size;
    if(n>header->count){
        n=header->count;
    }
    void* new_array=darray_init(n, elem_size, header->compare_func);
    if(!new_array){
        fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
        if(free_array) darray_free(array);
        return NULL;
    }
    memcpy(new_array, array, n*elem_size);
    CAST_HEADER(new_array)->count=n;
    if(free_array){
        darray_free(array);
    }
    return new_array;
}

void* _darray_skip(void* array,size_t n,bool free_array){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t elem_size=CAST_HEADER(array)->elem_size;
    if(n>header->count){
        n=header->count;
    }
    void* new_array=darray_init(header->count-n, elem_size, header->compare_func);
    if(!new_array){
        fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
        if(free_array) darray_free(array);
        return NULL;
    }
    memcpy(new_array, get_elem(array, n,elem_size), (header->count-n)*elem_size);
    CAST_HEADER(new_array)->count=header->count-n;
    if(free_array){
        darray_free(array);
    }
    return new_array;
}

void _darray_reduce(void* array, void* accumulator, void (*reduce_func)(void* acc, void* elem),bool free_array){
    if (!darray_is_valid(array)) return;
    if (!accumulator) {
        if (free_array) darray_free(array);
        return;
    }
    size_t len=darray_len(array);
    size_t elem_size=CAST_HEADER(array)->elem_size;
    for(size_t i=0;i<len;i++){
        reduce_func(accumulator,get_elem(array, i, elem_size));
    }
    if(free_array){
        darray_free(array);
    }
}

void* darray_reverse(void* array){
    if (!darray_is_valid(array) || darray_len(array) == 0) return array;
    size_t l=0,r=darray_len(array)-1;
    size_t elem_size=CAST_HEADER(array)->elem_size;
    void* temp = malloc(elem_size);
    if (!temp) {
        fputs("darray_reverse(): Failed to allocate temp for reversing.\n", stderr);
        return array; 
    }
    while(l<r){
        swap(get_elem(array, l, elem_size), get_elem(array, r, elem_size),temp, elem_size);
        l++;
        r--;
    }
    free(temp);
    return array;
}

void* _darray_min(void* array,bool free_array){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t elem_size=header->elem_size;
    if(header->count == 0 || header->compare_func == NULL) {
        if(free_array) darray_free(array);
        return NULL;
    }
    size_t len=darray_len(array);
    void* min=get_elem(array, 0, elem_size);
    for(size_t i=1;i<len;i++){
        if(header->compare_func(min,get_elem(array,i,elem_size))>0){
            min=get_elem(array,i,elem_size);
        }
    }
    if(free_array){
        void* safe_min = malloc(elem_size);
        if (safe_min) memcpy(safe_min, min, elem_size);
        darray_free(array);
        return safe_min; //caller is responsible for freeing this
    }
    return min;
}
void* _darray_max(void* array,bool free_array){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header=CAST_HEADER(array);
    size_t elem_size=header->elem_size;
    if(header->count==0 || header->compare_func==NULL ) {
        if(free_array) darray_free(array);
        return NULL;
    }
    size_t len=darray_len(array);
    void* max=get_elem(array, 0, elem_size);
    for(size_t i=1;i<len;i++){
        if(header->compare_func(max,get_elem(array,i,elem_size))<0){
            max=get_elem(array,i,elem_size);
        }
    }
    if(free_array){
        void* safe_max = malloc(elem_size);
        if (safe_max) memcpy(safe_max, max, elem_size);
        darray_free(array);
        return safe_max; //caller is responsible for freeing this
    }
    return max;
}

void* _darray_distinct(void* array,bool free_array){
    if (!darray_is_valid(array)) return NULL;
    DynArrayHeader* header = CAST_HEADER(array);
    size_t elem_size=header->elem_size;
    if (!header->compare_func) {
        if(free_array) darray_free(array);
        return NULL;
    }

    if (header->count <= 1) {
        // we pass it along instead of freeing it
        if(free_array) return array;
        void* ret_array= darray_copy(array);
        if(!ret_array){
            fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
            if(free_array) darray_free(array);
            return NULL;
        }
        return ret_array;
    }

    void* distinct_array = darray_copy(array);
    if(!distinct_array){
        fprintf(stderr,"Error: Memory allocation failed in function %s.\n",__FUNCTION__);
        if(free_array) darray_free(array);
        return NULL;
    }
    darray_sort(distinct_array);
    
    size_t write_idx = 1; // the next place to write a unique element
    size_t len = CAST_HEADER(distinct_array)->count;

    for (size_t read_idx = 1; read_idx < len; read_idx++) {
        void* prev = get_elem(distinct_array, write_idx - 1, elem_size);
        void* curr = get_elem(distinct_array, read_idx, elem_size);
        
        if (header->compare_func(prev, curr) != 0) {
            if (write_idx != read_idx) {
                memcpy(get_elem(distinct_array, write_idx, elem_size), curr, elem_size);
            }
            write_idx++;
        }
    }

    CAST_HEADER(distinct_array)->count = write_idx;

    if (free_array) darray_free(array);
    return distinct_array;
}

void* darray_set_compare(void* array, int (*compare_func)(void* elem1, void* elem2)){
    if (!darray_is_valid(array)) return NULL;
    
    DynArrayHeader* header = CAST_HEADER(array);
    header->compare_func = compare_func;
    
    return array; 
}
