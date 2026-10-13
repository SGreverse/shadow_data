#include "dyn_array.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
void map_func(void* dest_elem,void* src_elem){
    int value=*((int*)src_elem);
    *(float*)dest_elem=value*1.342;
}
bool filter_func(void* elem){
    return (*(int*)elem)>11;
}
int compare_func(void* elem1,void* elem2){
    return *((int*)elem1)-*((int*)elem2);
}
void reduce_func(void* accum,void* elem){
    (*(float*)accum)+=*(float*)elem;
}
int main(){

    int* arr=darray_init(4, sizeof(int), compare_func);
    darray_push(arr, 1);
    if(darray_contains(arr,int,1)){
        puts("YESSIR");
    }
    


    
}