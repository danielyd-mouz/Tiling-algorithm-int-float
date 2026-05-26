#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <gmp.h>
#include <mpfr.h>
#include "../header/Tiling_function_Simplified.h"


//Helper function to calculate the offset for the uniform distribution of values
static uint64_t uniform_offset(uint64_t max_offset, size_t i, size_t num)
{
    if (num == 1) {
        return max_offset / 2;
    }

    return (uint64_t)(((__uint128_t)max_offset * i) / (num - 1));
}

// Helper function to help determine of the number is already in the array
static bool is_in_uarray(uint64_t *array, size_t *length, uint64_t value)
{
    for (size_t i = 0; i < *length; i++) {
        if (array[i] == value) {
            return true;
        }
    }
    return false;
}

// Helper function to add the value to the array if it is not already in the array
static void add_to_uarray(uint64_t *array, size_t *length, size_t capacity, uint64_t value)
{
    if(*length >= capacity){
        return;
    }
    if (!is_in_uarray(array, length, value)) {
        array[*length] = value;
        (*length)++;
    }
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

    size_t count = 0;

    //special cases to be considered first
    add_to_uarray(result, &count, num, 0);
    add_to_uarray(result, &count, num, max_value);

    add_to_uarray(result, &count, num, 1);
    add_to_uarray(result, &count, num, max_value - 1);
    add_to_uarray(result, &count, num, max_value / 2);

    //places for powers of 2 and their complements to be considered next
    for(uint32_t i = 1; i< precision && count < num; i++){
        uint64_t value = 1ULL << i;
        add_to_uarray(result, &count, num, value);

        if(count < num){
            add_to_uarray(result, &count, num, max_value - value);
        }
        else{
            break;
        }
    }

    //places for all remaining values spaced evenly
    size_t remaining = num - count;
    for(size_t i = 0; i < remaining; i++){
        add_to_uarray(result, &count, num, uniform_offset(max_value, i, num));
    }

    //to fill in any remaining slots if there are duplicates
    size_t index = 0;
    size_t max_attempts = num > SIZE_MAX / 2 ? SIZE_MAX : num * 2; // to avoid overflow and limit the number of attempts
    while(count < num && index < max_attempts){                    // limit the number of attempts to avoid infinite loop
        uint64_t value = uniform_offset(max_value, index, max_attempts);
        add_to_uarray(result, &count, num, value);
        index ++;
    }

    //sequential values to fall back
    uint64_t val = 0;
    while(count < num && val <= max_value){
        add_to_uarray(result, &count, num, val);
        if(val == max_value){
            break;
        }
        val++;
    }

    return result;
}

// Helper function to determine if the number is already in the array
static bool is_in_iarray(int64_t *array, size_t *length, int64_t value)
{
    for (size_t i = 0; i < *length; i++) {
        if (array[i] == value) {
            return true;
        }
    }
    return false;
}

// Helper function to add the value to the array if it is not already in the array
static void add_to_iarray(int64_t *array, size_t *length,size_t capacity, int64_t value)
{
    if(*length >= capacity){
        return;
    }
    if (!is_in_iarray(array, length, value)) {
        array[*length] = value;
        (*length)++;
    }
}

// Helper funtion to convert uint64_t to int64_t
static int64_t uint64_to_int64(uint64_t value, int64_t min_value, uint32_t precision)
{
    if(precision == 64){
        int half = 1ULL << 63;
        if (value < half) {
            return (int64_t)value + min_value;
        } else {
            return (int64_t)(value - half);
        }
    }
    return (int64_t)value + min_value;
}

// Helper function to generate array of samples for signed int types
static int64_t *signed_tiling_int(size_t num, uint32_t precision)
{
    int64_t *result = xcalloc(num, sizeof(int64_t));

    if (num == 1) {
        result[0] = 0;
        return result;
    }

    uint64_t magnitude;
    int64_t max_value;
    int64_t min_value;
    uint64_t max_offset;

    if (precision == 64) {
        magnitude = 1ULL << 63;
        max_value = INT64_MAX;
        min_value = INT64_MIN;
        max_offset = UINT64_MAX;
    }else{
        magnitude = 1ULL << (precision - 1);
        max_value = (int64_t)(magnitude - 1);
        min_value = -max_value - 1;
        max_offset = (1ULL << precision) - 1;
    }

    size_t count = 0;
    
    //Special cases to be considered first
    add_to_iarray(result, &count, num, min_value);
    add_to_iarray(result, &count, num, max_value);
    add_to_iarray(result, &count, num, 0);

    add_to_iarray(result, &count, num, -1);
    add_to_iarray(result, &count, num, 1);
    
    add_to_iarray(result, &count, num, min_value + 1);
    add_to_iarray(result, &count, num, max_value - 1);
    add_to_iarray(result, &count, num, max_value / 2);

    for(uint32_t i = 1; i +1 < precision && count < num; i++){
        int64_t value = (int64_t)(1ULL << i);
        add_to_iarray(result, &count, num, value);
        add_to_iarray(result, &count, num, -value);
    }

    //Places for all remaining values spaced evenly
    size_t remaining = num - count;
    for (size_t i = 0; i < remaining; i++) {
        uint64_t offset = uniform_offset(max_offset, i, num);
        add_to_iarray(result, &count, num, uint64_to_int64(offset, min_value, precision));
    }

    //To fill in any remaining slots if there are duplicates
    size_t index = 0;
    size_t max_attempts = num > SIZE_MAX / 2 ? SIZE_MAX : num * 2;
    while (count < num && index < max_attempts) { // limit the number of attempts to avoid infinite loop
        uint64_t offset = uniform_offset(max_offset, index, max_attempts);
        add_to_iarray(result, &count, num, uint64_to_int64(offset, min_value, precision));
        index++;
    }

    int64_t val = min_value;
    while (count < num && val <= max_value) { // fill in any remaining slots with sequential values
        add_to_iarray(result, &count, num, val);
        if (val == max_value) {
            break;
        }
        val++;
    }

    return result;
}

//return a generic pointer containing the type of int the user is looking for. 
//The caller can cast the pointer to the correct type based on the input parameters.
void *tiling_int_simplified(size_t num, uint32_t precision, bool sign)
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

    // Check if the number of samples exceeds the number of available values for the specified precision
    if (precision < sizeof(size_t) * CHAR_BIT) {
        size_t max_samples = ((size_t)1) << precision;
        
        if (num > max_samples) {
            printf("Number of samples exceeds number of available values.\n");
            return NULL;
        }
    }

    // Check for potential overflow when allocating memory
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

//comparing two float numbers in bit level
static bool float_equal(float a, float b)
{
    uint32_t a_bits, b_bits;
    memcpy(&a_bits, &a, sizeof(float));
    memcpy(&b_bits, &b, sizeof(float));
    return a_bits == b_bits;
}

// Helper function to determine if the number is already in the float array
static bool is_in_farray(float *array, size_t *length, float value)
{
    for (size_t i = 0; i < *length; i++) {
        if (float_equal(array[i], value)) {
            return true;
        }
    }
    return false;
}

// Helper function to add the value to the array if it is not already in the float array
static void add_to_farray(float *array, size_t *length,size_t capacity, float value)
{
    if(*length >= capacity){
        return;
    }
    if (!is_in_farray(array, length, value)) {
        array[*length] = value;
        (*length)++;
    }
}

static float float_from_bits(uint32_t bits)
{
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void fill_float32_by_bits(float *result, size_t *count, size_t capacity)
{
    uint32_t max_positive_finite_bits = 0x7F7FFFFFu;

    size_t remaining = capacity - *count;
    size_t max_attempts = remaining > SIZE_MAX / 4 ? SIZE_MAX : remaining * 4;

    if (max_attempts < 2) {
        max_attempts = 2;
    }

    // First: representation-space uniform samples
    for (size_t i = 0; *count < capacity && i < max_attempts; i++) {
        uint32_t bits =
            (uint32_t)uniform_offset(max_positive_finite_bits, i, max_attempts);

        add_to_farray(result, count, capacity, float_from_bits(bits));

        if (*count < capacity) {
            add_to_farray(result, count, capacity,
                          float_from_bits(bits | 0x80000000u));
        }
    }

    // Second: sequential bit-pattern fallback
    for (uint32_t bits = 0; *count < capacity; bits++) {
        add_to_farray(result, count, capacity, float_from_bits(bits));

        if (*count < capacity) {
            add_to_farray(result, count, capacity,
                          float_from_bits(bits | 0x80000000u));
        }

        if (bits == max_positive_finite_bits) {
            break;
        }
    }
}

// Helper function that return an array of samples for float32 type.
static float *tiling_float32(size_t num)
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

    float *result = xcalloc(num,sizeof(float));

    if(num == 1){
        result[0] = 1.0f;
        return result;
    }

    
    size_t count = 0;

    //Special cases to be considered first
    add_to_farray(result, &count, num, 0.0f);
    add_to_farray(result, &count, num, -0.0f);

    add_to_farray(result, &count, num, 1.0f);
    add_to_farray(result, &count, num, -1.0f);
    add_to_farray(result, &count, num, 0.5f);
    add_to_farray(result, &count, num, -0.5f);

    add_to_farray(result, &count, num, FLT_EPSILON);
    add_to_farray(result, &count, num, -FLT_EPSILON);
    add_to_farray(result, &count, num, 1.0f + FLT_EPSILON);
    add_to_farray(result, &count, num, -(1.0f + FLT_EPSILON));

    add_to_farray(result, &count, num, FLT_MAX);
    add_to_farray(result, &count, num, -FLT_MAX);
    add_to_farray(result, &count, num, nextafterf(FLT_MAX, 0.0f));
    add_to_farray(result, &count, num, nextafterf(-FLT_MAX, 0.0f));

    add_to_farray(result, &count, num, FLT_MIN);
    add_to_farray(result, &count, num, -FLT_MIN);
    add_to_farray(result, &count, num, nextafterf(FLT_MIN, 0.0f));
    add_to_farray(result, &count, num, nextafterf(-FLT_MIN, 0.0f));

    add_to_farray(result, &count, num, nextafterf(0.0f, 1.0f));
    add_to_farray(result, &count, num, nextafterf(0.0f, -1.0f));

    //Places for all remaining values spaced evenly
    size_t num_pair = (num - count + 1) / 2;

    for(size_t i = 0; i<num_pair && count < num; i++){
        int exp;
        if(num_pair == 1){
            exp = 0;
        }
        else{
            exp = min_exp + (int)(((__uint128_t)i * exp_range) / (num_pair - 1));
        }
        add_to_farray(result, &count, num, ldexpf(1.0f, exp));

        if(count < num){
            add_to_farray(result, &count, num, -ldexpf(1.0f, exp));
        }
    }

    //To fill in any remaining slots if there are duplicates
    size_t index = 0;
    size_t max_attempts = num > SIZE_MAX / 2 ? SIZE_MAX : num * 2;
    while(count < num && index < max_attempts){ // limit the number of attempts to avoid infinite loop
        int exp = min_exp + uniform_offset(exp_range, index, max_attempts);
        add_to_farray(result, &count, num, ldexpf(1.0f, exp));
        if(count < num){
            add_to_farray(result, &count, num, -ldexpf(1.0f, exp));
        }
        index ++;
    }

    // If there are still remaining slots, fill them with sequential values
    fill_float32_by_bits(result, &count, num);

    if (count < num) {
        printf("Failed to generate enough unique float32 samples.\n");
        free(result);
        return NULL;
    }

    return result;
}

//comparing two double numbers in bit level
static bool double_equal(double a, double b)
{
    uint64_t a_bits, b_bits;
    memcpy(&a_bits, &a, sizeof(double));
    memcpy(&b_bits, &b, sizeof(double));
    return a_bits == b_bits;
}
//Helper function to determine if the number is already in the double array
static bool is_in_darray(double *array, size_t *length, double value)
{
    for (size_t i = 0; i < *length; i++) {
        if (double_equal(array[i], value)) {
            return true;
        }
    }
    return false;
}

// Helper function to add the value to the array if it is not already in the double array
static void add_to_darray(double *array, size_t *length,size_t capacity, double value)
{
    if(*length >= capacity){
        return;
    }
    if (!is_in_darray(array, length, value)) {
        array[*length] = value;
        (*length)++;
    }
}

static double double_from_bits(uint64_t bits)
{
    double value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void fill_float64_by_bits(double *result, size_t *count, size_t capacity)
{
    uint64_t max_positive_finite_bits = 0x7FEFFFFFFFFFFFFFULL;

    size_t remaining = capacity - *count;
    size_t max_attempts = remaining > SIZE_MAX / 4 ? SIZE_MAX : remaining * 4;

    if (max_attempts < 2) {
        max_attempts = 2;
    }

    // First: representation-space uniform samples
    for (size_t i = 0; *count < capacity && i < max_attempts; i++) {
        uint64_t bits =
            uniform_offset(max_positive_finite_bits, i, max_attempts);

        add_to_darray(result, count, capacity, double_from_bits(bits));

        if (*count < capacity) {
            add_to_darray(result, count, capacity,
                          double_from_bits(bits | 0x8000000000000000ULL));
        }
    }

    // Second: sequential bit-pattern fallback
    for (uint64_t bits = 0; *count < capacity; bits++) {
        add_to_darray(result, count, capacity, double_from_bits(bits));

        if (*count < capacity) {
            add_to_darray(result, count, capacity,
                          double_from_bits(bits | 0x8000000000000000ULL));
        }

        if (bits == max_positive_finite_bits) {
            break;
        }
    }
}

// Helper function that return an array of samples for float64 type.
static double *tiling_float64(size_t num)
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

    double *result = xcalloc(num,sizeof(double));

    if(num == 1){
        result[0] = 1.0;
        return result;
    }

    size_t count = 0;
    //Special cases to be considered first
    add_to_darray(result, &count, num, 0.0);
    add_to_darray(result, &count, num, -0.0);
    add_to_darray(result, &count, num, 1.0);
    add_to_darray(result, &count, num, -1.0);
    add_to_darray(result, &count, num, 0.5);
    add_to_darray(result, &count, num, -0.5);

    add_to_darray(result, &count, num, DBL_EPSILON);
    add_to_darray(result, &count, num, -DBL_EPSILON);
    add_to_darray(result, &count, num, 1.0 + DBL_EPSILON);
    add_to_darray(result, &count, num, -(1.0 + DBL_EPSILON));

    add_to_darray(result, &count, num, DBL_MAX);
    add_to_darray(result, &count, num, -DBL_MAX);
    add_to_darray(result, &count, num, nextafter(DBL_MAX, 0.0));
    add_to_darray(result, &count, num, nextafter(-DBL_MAX, 0.0));

    add_to_darray(result, &count, num, DBL_MIN);
    add_to_darray(result, &count, num, -DBL_MIN);
    add_to_darray(result, &count, num, nextafter(DBL_MIN, 0.0));
    add_to_darray(result, &count, num, nextafter(-DBL_MIN, 0.0));

    add_to_darray(result, &count, num, nextafter(0.0, 1.0));
    add_to_darray(result, &count, num, nextafter(0.0, -1.0));

    //Places for all remaining values spaced evenly

    size_t num_pair = ((num-count) + 1) / 2;
    for(size_t i = 0; i<num_pair && count < num; i++){
        int exp = 0;
        if(num_pair > 1){
            exp = min_exp + (int)(((__uint128_t)i * exp_range) / (num_pair - 1));
        }

        add_to_darray(result, &count, num, ldexp(1.0, exp));

        if(count < num){
            add_to_darray(result, &count, num, -ldexp(1.0, exp));
        }
    }

    size_t index = 0;
    size_t max_attempts = num > SIZE_MAX / 2 ? SIZE_MAX : num * 2;
    while(count < num && index < max_attempts){ // limit the number of attempts to avoid infinite loop
        int exp = min_exp + uniform_offset(exp_range, index, max_attempts);
        add_to_darray(result, &count, num, ldexp(1.0, exp));
        if(count < num){
            add_to_darray(result, &count, num, -ldexp(1.0, exp));
        }
        index ++;
    }

    // If there are still remaining slots, fill them with sequential values
    fill_float64_by_bits(result, &count, num);

    if (count < num) {
        printf("Failed to generate enough unique float64 samples.\n");
        free(result);
        return NULL;
    }

    return result;
}

//return a generic pointer containing the type of float the user is looking for. 
//The caller can cast the pointer to the correct type based on the input parameters.
void *tiling_float_simplified(size_t num, uint32_t precision)
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
        return tiling_float32(num);
    }
    else{
        return tiling_float64(num);
    }
}

