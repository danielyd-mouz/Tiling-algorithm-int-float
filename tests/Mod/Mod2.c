#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <string.h>

int moduler(void **inputs, size_t num_inputs, void *output){
    if (num_inputs != 2) return -1;
    int p = *((int64_t *)inputs[0]);
    int d = *((int64_t *)inputs[1]);
    if (d = 0){
        *((int64_t *)output) = INT64_MIN;
        return 0;
    }
    *((int64_t *)output) = p - (p/d) * d;
    return 0;
}