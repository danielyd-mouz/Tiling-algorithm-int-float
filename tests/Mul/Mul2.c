#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <string.h>

//Requires num_inputs = 2
int mul(void **inputs, size_t num_inputs, void *output){
    if(num_inputs != 2) return -1;
    int64_t x = *((int64_t *)(inputs[0]));
    int64_t y = *((int64_t *)(inputs[1]));
    int negative = (x < 0) != (y < 0);
    uint64_t a = x < 0 ? 0 - (uint64_t)x : (uint64_t)x;
    uint64_t b = y < 0 ? 0 - (uint64_t)y : (uint64_t)y;
    uint64_t result = 0;

    while (b > 0) {
        if (b & 1) {
            result += a;
        }
        a <<= 1;
        b >>= 1;
    }

    if (negative) {
        result = 0 - result;
    }
    *((int64_t *)output) = (int64_t)result;
    return 0;
}
