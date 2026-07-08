#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <string.h>

//Requires num_inputs = 2
int add(void **inputs, size_t num_inputs, void *output){
    if(num_inputs != 2) return -1;
    int64_t x = *((int64_t *)(inputs[0]));
    int64_t y = *((int64_t *)(inputs[1]));
    if (y >0){
        while (y >0){
            x +=1;
            y --;
        }
    }
    else{
        while (y <0){
            x -=1;
            y ++;
        }
    }
    *((int64_t *)output) = x;
    return 0;
}