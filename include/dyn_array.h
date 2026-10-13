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
