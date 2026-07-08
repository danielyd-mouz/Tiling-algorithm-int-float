#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <string.h>

//Requires num_inputs = 2
int divide(void **inputs, size_t num_inputs, void *output){
    if(num_inputs != 2) return -1;
    int64_t x = *((int64_t *)(inputs[0]));
    int64_t y = *((int64_t *)(inputs[1]));
    if(y == 0){
        *((int64_t *)output) = 0;
        return 0;
    }
    if(x == INT64_MIN && y == -1){
        *((int64_t *)output) = INT64_MIN;
        return 0;
    }
    *((int64_t *)output) = x / y;
    return 0;
}
