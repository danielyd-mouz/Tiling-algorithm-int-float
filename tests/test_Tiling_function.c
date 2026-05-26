#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <float.h>

#include "../header/Tiling_function.h"

typedef bool equal_function(void *a, void *b);

//Helper function to check if two float values are equal in bit level
static bool float_equal(void *a, void *b)
{
    uint32_t a_bits, b_bits;
    memcpy(&a_bits, (float *)a, sizeof(float));
    memcpy(&b_bits, (float *)b, sizeof(float));
    return a_bits == b_bits;
}

// Helper function to determine if two double values are equal in bit level
static bool double_equal(void *a, void *b)
{
    uint64_t a_bits, b_bits;
    memcpy(&a_bits, (double *)a, sizeof(double));
    memcpy(&b_bits, (double *)b, sizeof(double));
    return a_bits == b_bits;
}

// Helper function to determine if two unsigned int values are equal
static bool uint64_equal(void *a, void *b)
{
    return *(uint64_t *)a == *(uint64_t *)b;
}

// Helper function to determine if two signed int values are equal
static bool int64_equal(void *a, void *b)
{
    return *(int64_t *)a == *(int64_t *)b;
}

// Helper function to determine if all the values in the array are unique
bool all_unique(void *array, size_t length, size_t element_size, equal_function *equal_func)
{
    for (size_t i = 0; i < length; i++) {
        for (size_t j = i + 1; j < length; j++) {
            if (equal_func((void *)((char *)array + i * element_size), 
                           (void *)((char *)array + j * element_size))) 
            {
                return false;
            }
        }
    }
    return true;
}

// Helper function to determine if all the values are within the specified range for unsigned int types
bool within_range_uint64(uint64_t *array, size_t length, uint32_t precision)
{
    uint64_t max_value = (precision == 64) ? UINT64_MAX : (1ULL << precision) - 1;
    for (size_t i = 0; i < length; i++) {
        if (array[i] > max_value) {
            return false;
        }
    }
    return true;
}

// Helper function to determine if all the values are within the specified range for signed int types
bool within_range_int64(int64_t *array, size_t length, uint32_t precision)
{
    int64_t max_value = (precision = 64) ? MAX_INT64 : (1LL << (precision - 1)) - 1;
    int64_t min_value = (precision == 64) ? MIN_INT64 : -(1LL << (precision - 1));
    for (size_t i = 0; i < length; i++) {
        if (array[i] > max_value || array[i] < min_value) {
            return false;
        }
    }
    return true;
}

// Helper function to determine if all the values are within the specified range for float types
bool within_range_float(float *array, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        if (isnan(array[i]) || isinf(array[i])) {
            return false;
        }
    }
    return true;
}

void main(){
//contain the correct range of value for int types







//do not contain repetitive values







//contain the correct range of value for float types







}