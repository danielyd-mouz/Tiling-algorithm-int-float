#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include "../header/Tiling_function.h"

//Helper function to conver uint64_t value to int64_t value
static uint64_t uniform_offset(uint64_t max_offset, size_t i, size_t num)
{
    if (num == 1) {
        return max_offset / 2;
    }

    return (uint64_t)(((__uint128_t)max_offset * i) / (num - 1));
}

// Helper function to generate array of samples for unsigned int types
static uint64_t *unsigned_tiling_int(size_t num, uint32_t precision)
// REQUIRES: num > 0;
// REQUIRES: precision <=64;
// num <= number of available values for the specified precision;
// ENSURES: result != NULL && \length(result) == num;
{
    uint64_t *result = xcalloc(num,sizeof(uint64_t));
    uint64_t max_value;

    if(precision == 64){
        max_value = UINT64_MAX;
    }
    else{
        max_value = (1ULL << precision) - 1;
    }
    for(size_t i = 0; i < num; i++){
        result[i] = uniform_offset(max_value, i, num);
    }
    return result;
}

// Helper function to convert uint64_t offset to int64_t value for 64-bit precision
static int64_t signed64_from_offset(uint64_t offset)
{
    uint64_t half = 1ULL << 63;

    if (offset < half) {
        return INT64_MIN + (int64_t)offset;
    } else {
        return (int64_t)(offset - half);
    }
}

// Helper function to generate array of samples for signed int types
static int64_t *signed_tiling_int(size_t num, uint32_t precision)
{
    int64_t *result = xcalloc(num, sizeof(int64_t));

    if (num == 1) {
        result[0] = 0;
        return result;
    }

    if (precision == 64) {
        for (size_t i = 0; i < num; i++) {
            uint64_t offset = uniform_offset(UINT64_MAX, i, num);
            result[i] = signed64_from_offset(offset);
        }

        return result;
    }

    uint64_t magnitude = 1ULL << (precision - 1);
    int64_t max_value = (int64_t)(magnitude - 1);
    int64_t min_value = -max_value - 1;
    uint64_t max_offset = (uint64_t)(max_value - min_value);

    for (size_t i = 0; i < num; i++) {
        uint64_t offset = uniform_offset(max_offset, i, num);
        result[i] = (int64_t)((__int128_t)min_value + offset);
    }

    return result;
}

//return a generic pointer containing the type of int the user is looking for. 
//The caller can cast the pointer to the correct type based on the input parameters.
void *tiling_int(size_t num, uint32_t precision, bool sign)
// REQUIRES: num > 0;
// REQUIRES: 0<precision && precision <= 64;
// REQUIRES: num <= number of representable values for the specified precision;
// ENSURES: result != NULL && \length(result) == num;
{
    // Validate input parameters
    if(precision == 0 || precision > 64){
        printf("Precision input must be between 1 and 64.\n");
        return NULL;
    }
    if (num == 0) {
        printf("Number of samples must be greater than 0.\n");
        return NULL;
    }
    if (precision < sizeof(size_t) * CHAR_BIT) {
        size_t max_samples = ((size_t)1) << precision;
        
        if (num > max_samples) {
            printf("Number of samples exceeds number of available values.\n");
            return NULL;
        }
    }
    if(num > SIZE_MAX/sizeof(uint64_t)){
        printf("Number of samples exceeds the maximum allowed.\n");
        return NULL;
    } 

    // Call the appropriate helper function based on the sign parameter
    if(!sign){
        return (void *)unsigned_tiling_int(num, precision);
    }
    else{
        return (void *)signed_tiling_int(num, precision);
    }
}

/*----------------------------------------------------------------------------------------------------------
float types tiling function. The function will return an array of samples for the specified precision (32 or 64 bits).
----------------------------------------------------------------------------------------------------------*/

// Helper function that return an array of samples for float32 type.
static float *tiling_float32(size_t num, uint32_t precision)
// REQUIRES: num > 0;
// REQUIRES: precision == 32;
// REQUIRES num <= number of available values for the specified precision;
// ENSURES: result != NULL && \length(result) == num;
{
    if(num > SIZE_MAX/sizeof(float)){
        printf("Number of samples exceeds the maximum allowed for float type.\n");
        return NULL;
    }

    int min_exp = FLT_MIN_EXP - FLT_MANT_DIG;
    int max_exp = FLT_MAX_EXP - 1;
    int exp_range = max_exp - min_exp;

    if(num > 2 * (size_t)(exp_range+1)){
        printf("Number of samples exceeds number of available values for the specified precision.\n");
        return NULL;
    }

    float *result = xcalloc(num,sizeof(float));

    if(num == 1){
        result[0] = 1.0f;
        return result;
    }

    size_t num_pair = (num + 1) / 2;
    size_t count = 0;

    for(size_t i = 0; i<num_pair && count < num; i++){
        int exp = 0;
        if(num_pair == 1){
            exp = 1.0f;
        }
        else{
            exp = min_exp + (i * exp_range) / (num_pair - 1);
        }
        result[count++] = ldexpf(1.0f, exp);

        if(count < num){
            result[count++] = -ldexpf(1.0f, exp);
        }
    }

    (void)precision; // to avoid unused parameter warning

    return result;
}

// Helper function that return an array of samples for float64 type.
static double *tiling_float64(size_t num, uint32_t precision)
// REQUIRES: num > 0;
// REQUIRES: precision == 64;
// REQUIRES num <= number of available values for the specified precision;
// ENSURES: result != NULL && \length(result) == num;
{
    if(num > SIZE_MAX/sizeof(double)){
        printf("Number of samples exceeds the maximum allowed for float type.\n");
        return NULL;
    }

    int min_exp = DBL_MIN_EXP - DBL_MANT_DIG;
    int max_exp = DBL_MAX_EXP - 1;
    int exp_range = max_exp - min_exp;

    if(num > 2 * (size_t)(exp_range+1)){
        printf("Number of samples exceeds number of available values for the specified precision.\n");
        return NULL;
    }

    double *result = xcalloc(num,sizeof(double));

    if(num == 1){
        result[0] = 1.0;
        return result;
    }

    size_t num_pair = (num + 1) / 2;
    size_t count = 0;

    for(size_t i = 0; i<num_pair && count < num; i++){
        int exp = 0;
        if(num_pair == 1){
            result[count++] = 1.0;
        }
        else{
            exp = min_exp + (i * exp_range) / (num_pair - 1);
        }

        result[count++] = ldexp(1.0, exp);

        if(count < num){
            result[count++] = -ldexp(1.0, exp);
        }
    }

    (void)precision; // to avoid unused parameter warning

    return result;
}

//return a generic pointer containing the type of float the user is looking for. 
//The caller can cast the pointer to the correct type based on the input parameters.
void *tiling_float(size_t num, uint32_t precision)
// REQUIRES: num > 0;
// REQUIRES: precision == 32 || precision == 64;
// ENSURES: result != NULL && \length(result) == num;
{
    // Validate input parameters
    if(precision != 32 && precision != 64){
        printf("Invalid precision input. Please input 32 or 64 for precision.\n");
        return NULL;
    }
    if(num == 0){
        printf("Number of samples must be greater than 0.\n");
        return NULL;
    }

    // Call the appropriate helper function based on the precision parameter
    if(precision == 32){
        return (void *)tiling_float32(num, precision);
    }
    else{
        return (void *)tiling_float64(num, precision);
    }
}

