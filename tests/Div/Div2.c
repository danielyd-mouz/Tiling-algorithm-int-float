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

    int negative = (x < 0) != (y < 0);
    uint64_t dividend = x < 0 ? 0 - (uint64_t)x : (uint64_t)x;
    uint64_t divisor = y < 0 ? 0 - (uint64_t)y : (uint64_t)y;
    uint64_t quotient = 0;

    for (int bit = 63; bit >= 0; bit--) {
        if ((dividend >> bit) >= divisor) {
            dividend -= divisor << bit;
            quotient |= UINT64_C(1) << bit;
        }
    }

    if (negative) {
        quotient = 0 - quotient;
    }
    *((int64_t *)output) = (int64_t)quotient;
    return 0;
}
