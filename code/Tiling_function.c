#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include "../header/type.h"
#include "../header/Tiling_function.h"

typedef enum{
    RNDN,
    RNDZ,
    RNDU,
    RNDD,
    RNDA
}rnd_type;

//Helper functions to do uniform sampling
void uniform_offset(mpz_t max_offset, mpz_t value, size_t i, size_t num)
{
    if (num == 1) {
        mpz_fdiv_q_ui(value, max_offset, 3);
        return;
    }
    mpz_t temp;
    mpz_init(temp);
    mpz_mul_ui(temp, max_offset, i);
    mpz_fdiv_q_ui(value, temp, (num - 1));
    mpz_clear(temp);
}

// Helper function to add the value to the unsigned int array if it is not already in the unsigned integer array
void add_to_uarray(mpz_t *array, size_t *length, size_t capacity, mpz_t value, uint32_t precision)
{
    if(*length >= capacity) return;
    mpz_t max;
    mpz_t min;
    mpz_t one;
    mpz_init2(one, precision+1);
    mpz_set_ui(one, 1);
    mpz_init2(max, precision+1);
    mpz_mul_2exp(max, one, precision);
    mpz_init(min);

    if(mpz_cmp(value, max) >= 0 || mpz_cmp(value,min)<0){
        mpz_clear(max);
        mpz_clear(min);
        mpz_clear(one);
        return;
    }

    if(!is_in_uarray(array, *length, value)){
      mpz_set(array[*length],value);
      *length ++;   
    }

    mpz_clear(max);
    mpz_clear(min);
    mpz_clear(one);
}

// Helper function to check if the value already exist in the unsigned int array
bool is_in_uarray(mpz_t *array, size_t length, mpz_t value)
{
    for(size_t i = 0; i<length; i++){
        if(mpz_cmp(value, array[i]) == 0) return true;
    }
    return false;
}

// Helper function to add boundary cases to unsigned int array
void add_boundary_ui(size_t num, uint32_t precision, size_t *length, mpz_t *result)
{
    mpz_t max;
    mpz_t min;
    mpz_t max_oneless;
    mpz_t min_onemore;
    mpz_t half;
    mpz_t one;
    mpz_init2(half, precision+1);
    mpz_init2(one, precision + 1);
    mpz_set_ui(one, 1);
    mpz_init2(max, precision+1);
    mpz_init2(min, precision+1);
    mpz_init2(max_oneless, precision+1);
    mpz_init2(min_onemore, precision+1);
    mpz_mul_2exp(max, one, precision);
    mpz_sub_ui(max, max, 1);
    mpz_sub_ui(max_oneless, max, 1);
    mpz_add_ui(min_onemore, min, 1);
    mpz_fdiv_q_ui(half, max, 2);
    add_to_uarray(result, length, num, max, precision);
    add_to_uarray(result, length, num, min, precision);
    add_to_uarray(result, length, num, max_oneless, precision);
    add_to_uarray(result, length, num, min_onemore, precision);
    add_to_uarray(result, length, num, half, precision);
    mpz_clear(max);
    mpz_clear(min);
    mpz_clear(one);
    mpz_clear(max_oneless);
    mpz_clear(min_onemore);
    mpz_clear(half);
}

// Sub-tiling function for unsigned int
void unsigned_tiling_int(size_t num, uint32_t precision, mpz_t *result)
{
    //initialize common numbers
    mpz_t max;
    mpz_t min;
    mpz_t one;
    mpz_init2(one, precision + 1);
    mpz_set_ui(one, 1);
    mpz_init2(max, precision+1);
    mpz_init2(min, precision+1);
    mpz_mul_2exp(max, one, precision);
    mpz_sub_ui(max, max, 1);

    if (num == 1) {
        mpz_init2(result[0],precision);
        mpz_clear(max);
        mpz_clear(min);
        mpz_clear(one);
        return;
    }

    size_t count = 0;
    
    add_boundary_ui(num, precision, &count, result);

    if(count == num) {
        mpz_clear(max);
        mpz_clear(min);
        mpz_clear(one);
        return;
    }

    for(uint32_t i = 1; i< precision && count < num; i++){
        mpz_t temp;
        mpz_init2(temp, precision);
        mpz_mul_2exp(temp,one,i);
        add_to_uarray(result, &count, num, temp, precision);

        if(count < num){
            mpz_t comp;
            mpz_init2(comp, precision);
            mpz_sub(comp, max, temp);
            add_to_uarray(result, &count, num, comp, precision);
            mpz_clear(comp);
            mpz_clear(temp);
        }
        else{
            mpz_clear(temp);
            break;
        }
    }

    mpz_t v;
    mpz_init2(v, precision);

    //Use uniform sampling to fill in the slot
    size_t remaining = num - count;
    for(size_t i = 0; i < remaining; i++){
        uniform_offset(max, v, i, num);
        add_to_uarray(result, &count, num, v, precision);
    }

    //Try to fill in the remaining slots with more uniform sampling
    size_t index = 0;
    size_t max_attempts = num > SIZE_MAX / 2 ? SIZE_MAX : num * 2; // to avoid overflow and limit the number of attempts
    while(count < num && index < max_attempts){                    // limit the number of attempts to avoid infinite loop
        uniform_offset(max, v, index, num);
        add_to_uarray(result, &count, num, v,precision);
        index ++;
    }

    //sequential values to fall back
    mpz_t iterate;
    mpz_init2(iterate,precision);
    while(count < num && mpz_cmp(iterate,max)<0){
        add_to_uarray(result, &count, num, iterate,precision);
        mpz_add_ui(iterate,iterate,1);
    }
    mpz_clear(max);
    mpz_clear(min);
    mpz_clear(one);
    mpz_clear(iterate);
    mpz_clear(v);
}

//Helper function to add the value to the signed int array
void add_to_iarray(mpz_t *array, size_t *length, size_t capacity, mpz_t value, uint32_t precision)
{
    if(*length >= capacity) return;
    mpz_t max;
    mpz_t min;
    mpz_t one;
    mpz_t neg_one;
    mpz_init2(one, precision);
    mpz_init2(neg_one, precision);
    mpz_set_ui(one, 1);
    mpz_sub(neg_one, neg_one, one);
    mpz_init2(max, precision);
    mpz_mul_2exp(max, one, precision-1);
    mpz_init(min);
    mpz_mul_2exp(min, neg_one, precision-1);

    if(mpz_cmp(value, max) >= 0 || mpz_cmp(value,min)<0) {
        mpz_clear(max);
        mpz_clear(min);
        mpz_clear(one);
        mpz_clear(neg_one);
        return;
    }

    if(!is_in_iarray(array, *length, value)){
      mpz_set(array[*length],value);
      *length ++;   
    }

    mpz_clear(max);
    mpz_clear(min);
    mpz_clear(one);
    mpz_clear(neg_one);
}

// Helper function to check if the value already exist in the signed array
bool is_in_iarray(mpz_t *array, size_t length, mpz_t value)
{
    for(size_t i = 0; i<length; i++){
        if(mpz_cmp(value, array[i]) == 0) return true;
    }
    return false;
}

// Helper function to add boundary cases
void add_boundary_si(size_t num, uint32_t precision, size_t *length, mpz_t *result)
{
    mpz_t max;
    mpz_t min;
    mpz_t max_oneless;
    mpz_t min_onemore;
    mpz_t one;
    mpz_t max_half;
    mpz_t min_half;
    mpz_t neg_one;
    mpz_init2(one, precision + 1);
    mpz_init2(max_half,precision);
    mpz_init2(min_half, precision);
    mpz_init2(neg_one, precision);
    mpz_set_ui(one, 1);
    mpz_sub(neg_one, neg_one, one);
    mpz_init2(max, precision+1);
    mpz_init2(min, precision+1);
    mpz_init2(max_oneless, precision+1);
    mpz_init2(min_onemore, precision+1);
    mpz_mul_2exp(max, one, precision-1);
    mpz_sub(min, min, max);
    mpz_sub_ui(max, max, 1);
    mpz_sub_ui(max_oneless, max, 1);
    mpz_add_ui(min_onemore, min, 1);
    mpz_fdiv_q_ui(max_half, max, 2);
    mpz_fdiv_q_ui(min_half, min, 2);

    add_to_iarray(result, length, num, max, precision);
    add_to_iarray(result, length, num, min, precision);
    add_to_iarray(result, length, num, max_oneless, precision);
    add_to_iarray(result, length, num, min_onemore, precision);
    add_to_iarray(result, length, num, max_half, precision);
    add_to_iarray(result, length, num, min_half, precision);
    add_to_iarray(result, length, num, one, precision);
    add_to_iarray(result, length, num, neg_one, precision);
    mpz_clear(max);
    mpz_clear(min);
    mpz_clear(max_oneless);
    mpz_clear(min_onemore);
    mpz_clear(min_half);
    mpz_clear(max_half);
    mpz_clear(one);
    mpz_clear(neg_one);
}

// sub-tiling function for signed int
void signed_tiling_int(size_t num, uint32_t precision, mpz_t *result)
{
    //initialize common numbers
    mpz_t max;
    mpz_t min;
    mpz_t one;
    mpz_t max_offset;
    
    mpz_init2(one, precision + 1);
    mpz_set_ui(one, 1);
    mpz_init2(max, precision);
    mpz_init2(min, precision);
    mpz_mul_2exp(max, one, precision-1);
    mpz_sub(min, min, max);
    mpz_sub_ui(max, max, 1);
    mpz_init2(max_offset,precision+1);
    mpz_mul_2exp(max_offset, one, precision);
    mpz_sub(max_offset, max_offset, one);

    if (num == 1) {
        mpz_init2(result[0],precision);
        mpz_clear(max);
        mpz_clear(min);
        mpz_clear(one);
        mpz_clear(max_offset);
        return;
    }

    size_t count = 0;
    
    add_boundary_si(num, precision, &count, result);

    if(count == num) {
        mpz_clear(max);
        mpz_clear(min);
        mpz_clear(one);
        mpz_clear(max_offset);
        return;
    }

    for(uint32_t i = 1; i+1 < precision && count < num; i++){
        mpz_t temp;
        mpz_init2(temp, precision);
        mpz_mul_2exp(temp,one,i);
        add_to_iarray(result, &count, num, temp, precision);

        if(count < num){
            mpz_t comp;
            mpz_init2(comp, precision);
            mpz_sub(comp, comp, temp);
            add_to_iarray(result, &count, num, comp, precision);
            mpz_clear(comp);
            mpz_clear(temp);
        }
        else{
            mpz_clear(temp);
            break;
        }
    }

    mpz_t v;
    mpz_init2(v, precision);

    //Use uniform sampling to fill in the slot
    size_t remaining = num - count;
    for(size_t i = 0; i < remaining; i++){
        uniform_offset(max_offset, v, i, num);
        mpz_add(v, v, min);
        add_to_iarray(result, &count, num, v, precision);
    }

    //Try to fill in the remaining slots with more uniform sampling
    size_t index = 0;
    size_t max_attempts = num > SIZE_MAX / 2 ? SIZE_MAX : num * 2; // to avoid overflow and limit the number of attempts
    while(count < num && index < max_attempts){                    // limit the number of attempts to avoid infinite loop
        uniform_offset(max_offset, v, index, num);
        mpz_add(v, v, min);
        add_to_iarray(result, &count, num, v,precision);
        index ++;
    }

    //sequential values to fall back
    mpz_t iterate;
    mpz_init2(iterate,precision);
    mpz_set(iterate,min);
    while(count < num && mpz_cmp(iterate,max)<0){
        add_to_iarray(result, &count, num, iterate,precision);
        mpz_add_ui(iterate,iterate,1);
    }
    mpz_clear(max);
    mpz_clear(min);
    mpz_clear(one);
    mpz_clear(iterate);
    mpz_clear(v);
    mpz_clear(max_offset);
}

// Tiling function for mpz_t
void tiling_int(mpz_t **arr, size_t num, uint32_t precision, bool sign)
// REQUIRES: num > 0;
// REQUIRES: arr == NULL;
// REQUIRES: 0 < precision && precision <= 256;
// REQUIRES: preision > 64 || num <= (size_t)(1ULL << precision - 1);
// ENSURES: result != NULL && \length(result) == num;
{
    // Validate input parameters
    if(precision == 0 || precision > 256){
        printf("Precision input must be between 1 and 256.\n");
        return;
    }
    if (num == 0) {
        printf("Number of samples must be greater than 0.\n");
        return;
    }

    // Check if the number of samples exceeds the number of available values for the specified precision
    mpz_t max_samples;
    mpz_t base;
    mpz_init2(max_samples, 257);
    mpz_init2(base, 257);
    mpz_set_ui(base, 1);
    mpz_mul_2exp (max_samples, base, precision);
    mpz_t n;
    mpz_init(n);
    mpz_set_ui(n, num);
    if (mpz_cmp(n, max_samples) > 0) {
        printf("Number of samples exceeds number of available values.\n");
        mpz_clear(max_samples);
        mpz_clear(base);
        mpz_clear(n);
        return;
    }
    mpz_clear(max_samples);
    mpz_clear(base);
    mpz_clear(n);

    // Check for potential overflow when allocating memory
    if(num > SIZE_MAX/sizeof(mpz_t)){
        printf("Number of samples exceeds the maximum allowed.\n");
        return;
    }

    *arr = xmalloc(num*sizeof(mpz_t));

    // Call the appropriate helper function based on the sign parameter
    if(!sign){
        unsigned_tiling_int(num, precision, *arr);
        return;
    }
    else{
        signed_tiling_int(num, precision, *arr);
        return;
    }
}

// Helper function to clear the integer mpz_t array
void clear_iarray(mpz_t **array, size_t num)
// REQUIRES: array != NULL;
// REQUIRES: num = \length(array);
{
    for(size_t i = 0; i<num; i++){
        mpz_clear((*array)[i]);
    }
    free(*array);
}

// Helper function to check if the value already exist in the mpfr_t array
bool is_in_farray(mpfr_t *array, size_t length, mpfr_t value)
{
    for (size_t i = 0; i < length; i++) {
        if (mpfr_cmp(array[i], value) == 0) {
            return true;
        }
    }
    return false;
}

// Helper function to add the value to the array if it is not already in the mpfr_t array
void add_to_farray(mpfr_t *array, size_t *length, size_t capacity, mpfr_t value, mpfr_t max, mpfr_t min, mpfr_rnd_t rnd)
{
    if(*length >= capacity) return;

    if(mpfr_cmp(value, max) > 0) return;
    if(mpfr_cmp(value, min) < 0) return;

    if(!is_in_farray(array, *length, value)){
      mpfr_set(array[*length],value, rnd);
      *length ++;   
    }
}

// Helper function to add boundary cases
void add_boundary_f(size_t num, mpfr_prec_t precision, mpfr_prec_t mantissa, size_t *length, mpfr_t *result, mpfr_rnd_t rnd)
{
    mpfr_t max;
    mpfr_t min;
    mpfr_t max_less;
    mpfr_t min_more;
    mpfr_t max_half;
    mpfr_t min_half;
    mpfr_init2(max, mantissa);
    mpfr_init2(min, mantissa);
    mpfr_init2(max_less, mantissa);
    mpfr_init2(min_more, mantissa);
    mpfr_init2(max_half, mantissa);
    mpfr_init2(min_half, mantissa);
    mpfr_set_ui(max, 1, rnd);
    mpfr_div_2ui(max, max, mantissa, rnd);
    mpfr_ui_sub(max, 1, max, rnd);
    mpfr_mul_2si(max, max, precision - 1 - mantissa, rnd);
    mpfr_sub(min, min, max,rnd);

    add_to_farray(result, length, num, max, max, min, rnd);
    add_to_farray(result, length, num, min, max, min, rnd);
    mpfr_set(max_less, max, rnd);
    mpfr_set(min_more, min, rnd);
    mpfr_nextbelow(max_less);
    mpfr_nextabove(min_more);
    add_to_farray(result, length,num,max_less,max,min,rnd);
    add_to_farray(result,length,num,min_more,max,min,rnd);
    mpfr_div_ui(max_half,max , 2 ,rnd);
    add_to_farray(result,length,num,max_half,max,min,rnd);
    mpfr_div_ui(min_half,min , 2 ,rnd);
    add_to_farray(result,length,num,min_half,max,min,rnd);

    mpfr_clear(max);
    mpfr_clear(min);
    mpfr_clear(max_less);
    mpfr_clear(min_more);
    mpfr_clear(max_half);
    mpfr_clear(min_half);
}

// Helper functions to perform uniform sampling for mpfr_t type
void uniform_offset_f(mpfr_t max_offset, mpfr_t value, size_t i, size_t num, mpfr_rnd_t rnd)
{
    if (num == 1) {
        mpfr_div_ui(value, max_offset, 3, rnd);
        return;
    }
    mpfr_t temp;
    mpfr_init2(temp, mpfr_get_prec(max_offset));
    mpfr_mul_ui(temp, max_offset, i, rnd);
    mpfr_div_ui(value, temp, (num - 1), rnd);
    mpfr_clear(temp);
}

// Body of the tiling function for float types
void tiling_float(mpfr_t **arr, size_t num, mpfr_prec_t precision, mpfr_prec_t mantissa, rnd_type round)
// REQUIRES: num > 0;
// REQUIRES: num <= SIZE_MAX/(sizeof(mpfr_t));
// REQUIRES: arr == NULL;
// REQUIRES: precision >= 0 && precision <= 256;
// ENSURES: result != NULL && \length(result) == num;
{
    mpfr_rnd_t rnd;
    switch(round){
        case RNDN:
            rnd = MPFR_RNDN;
            break;
        case RNDZ:
            rnd = MPFR_RNDZ;
            break;
        case RNDU:
            rnd = MPFR_RNDU;
            break;
        case RNDD:
            rnd = MPFR_RNDD;
            break;
        case RNDA:
            rnd = MPFR_RNDA;
            break;
        default:
            rnd = MPFR_RNDN;
             break;
    }
    if(precision == 0 || precision > 256){
        printf("Precision input must be between 1 and 256.\n");
        return;
    }

    if (num == 0) {
        printf("Number of samples must be greater than 0.\n");
        return;
    }

    if(num > SIZE_MAX/sizeof(mpfr_t)){
        printf("Number of samples exceeds the maximum allowed for float type.\n");
        return;
    }

    *arr = xmalloc(num*sizeof(mpfr_t));

    mpfr_t max;
    mpfr_t min;
    mpfr_init2(max, mantissa);
    mpfr_init2(min, mantissa);
    mpfr_prec_t n = mantissa;
    mpfr_prec_t exp = precision -1 - mantissa;
    mpfr_set_ui(max, 1, rnd);
    mpfr_set_ui(min, 0, rnd);
    mpfr_div_2ui(max, max, n, rnd);
    mpfr_ui_sub(max, 1, max, rnd);
    mpfr_mul_2si(max, max, exp, rnd);
    mpfr_sub(min, min, max,rnd);

    if(num == 1){
        mpfr_init2((*arr)[0], mantissa);
        mpfr_set_ui((*arr)[0], 1, rnd);
        mpfr_clear(max);
        mpfr_clear(min);
        return;
    }

    size_t count = 0;
    //Special cases to be considered first
    mpfr_t zero;
    mpfr_init2(zero, mantissa);
    mpfr_set_ui(zero, 0, rnd);
    add_to_farray((*arr), &count, num, zero, max, min, rnd);
    add_boundary_f(num, precision, mantissa, &count, (*arr), rnd);

    //Place numbers with uniform exponent distribution
    mpfr_t exp2;
    mpfr_init2(exp2, mantissa);
    mpfr_set_ui(exp2, 1, rnd);
    while(count < num && mpfr_cmp(exp2, max) <= 0){
        add_to_farray((*arr), &count, num, exp2, max, min, rnd);
        if(count < num){
            mpfr_neg(exp2, exp2, rnd);
            add_to_farray((*arr), &count, num, exp2, max, min, rnd);
        }
        mpfr_mul_2ui(exp2, exp2, 1, rnd);
    }
    mpfr_clear(exp2);

    //Places for all remaining values spaced evenly
    size_t num_pair = ((num-count) + 1) / 2;
    mpfr_t temp;
    mpfr_init2(temp, mantissa);
    if(num_pair > 1){
        for(size_t i = 0; i<num_pair && count < num; i++){
            uniform_offset_f(max, temp, i, num_pair, rnd);
            add_to_farray((*arr), &count, num, temp, max, min, rnd);
            if(count < num){
                mpfr_neg(temp, temp, rnd);
                add_to_farray((*arr), &count, num, temp, max, min, rnd);
            }
        }
    }
    mpfr_clear(temp);

    //Fill in the array with numbers starting from 0 and increasing by a small step until the array is full
    mpfr_t step;
    mpfr_init2(step, mantissa);
    mpfr_set_zero(step, rnd);
    while(count < num && mpfr_cmp(step, max) <= 0){
        add_to_farray((*arr), &count, num, step, max, min, rnd);
        if(count < num){
            mpfr_neg(step, step, rnd);
            add_to_farray((*arr), &count, num, step, max, min, rnd);
        }
        mpfr_nextafter(step, step, rnd);
    }
    mpfr_clear(step);


    mpfr_clear(max);
    mpfr_clear(min);
}

// Helper function to clear the mpfr_t array
void clear_farray(mpfr_t **array, size_t num)
// REQUIRES: array != NULL;
// REQUIRES: num = \length(array);
{
    for(size_t i = 0; i<num; i++){
        mpfr_clear((*array)[i]);
    }
    free(*array);
}