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
#include "../header/entire_combination.h"

//new type that contains both the size of the sample array and also the array itself
typedef struct {
    size_t size;
    size_t elem_size;
    void *array;
} sample_array;

struct sample_array_list {
    size_t num_arrays;
    size_t counter;
    sample_array *samples;
};

//Lightweight handle — stores metadata + a reference to the input arrays.
//num_comb is mpz_t so the total combination count is unbounded.
//No combinations are materialized; get_the_ith_combination computes one on-the-fly.
struct combination_array {
    mpz_t num_comb;                // total number of combinations (arbitrary precision)
    size_t num_elem;               // number of elements in each combination
    sample_array_list *samples;    // reference to the input arrays (not owned)
};

//Helper function to create a sample_array_list struct
sample_array_list *create_sample_array_list(size_t num_arrays)
{
    if(num_arrays == 0) return NULL;
    if(num_arrays > SIZE_MAX / sizeof(sample_array)) {
        fprintf(stderr, "Error: Number of arrays exceeds maximum allowed.\n");
        return NULL;
    }

    sample_array_list *sample_list = malloc(sizeof(sample_array_list));
    if (sample_list == NULL) {
        fprintf(stderr, "Error: failed to allocate sample_array_list.\n");
        return NULL;
    }
    sample_list->num_arrays = num_arrays;
    sample_list->counter = 0;
    sample_list->samples = calloc(num_arrays, sizeof(sample_array));
    if (sample_list->samples == NULL) {
        fprintf(stderr, "Error: failed to allocate samples array.\n");
        free(sample_list);
        return NULL;
    }
    return sample_list;
}

//Helper function to put an array as a sample_array struct
void add_to_sample_array(sample_array_list *sample_list, size_t elem_size, size_t size, void *array)
{
    if(sample_list == NULL || sample_list->counter == sample_list->num_arrays){
        return;
    }
    if(elem_size == 0 || size == 0 || array == NULL){
        return;
    }
    sample_array sample;
    sample.size = size;
    sample.elem_size = elem_size;
    sample.array = array;
    sample_list->samples[sample_list->counter] = sample;
    sample_list->counter++;
}

//Lightweight "constructor" — computes the total number of combinations (as mpz_t) and
//stores a reference to the input arrays. No combinations are materialized; each call to
//get_the_ith_combination computes one combination on-the-fly via mixed-radix.
combination_array *combine_arrays(sample_array_list *sample_list)
{
    if(sample_list == NULL) return NULL;
    if(sample_list->counter != sample_list->num_arrays) {
        fprintf(stderr, "Error: sample_array_list is not fully populated.\n");
        return NULL;
    }

    size_t num = sample_list->num_arrays;
    if(num == 0) return NULL;

    mpz_t total_comb;
    mpz_init_set_ui(total_comb, 1UL);

    mpz_t sz_mpz;
    mpz_init(sz_mpz);

    for(size_t i = 0; i < sample_list->num_arrays; i++){
        size_t sz = sample_list->samples[i].size;
        if(sz == 0){
            mpz_clear(total_comb);
            mpz_clear(sz_mpz);
            return NULL;
        }
        // Convert size_t to mpz_t portably (mpz_set_ui only takes unsigned long,
        // which is 32 bits on 64-bit Windows; mpz_import handles any size_t).
        mpz_import(sz_mpz, 1, -1, sizeof(size_t), 0, 0, &sz);
        mpz_mul(total_comb, total_comb, sz_mpz);
    }
    mpz_clear(sz_mpz);

    combination_array *comb_array = malloc(sizeof(combination_array));
    if (comb_array == NULL) {
        fprintf(stderr, "Error: failed to allocate combination_array.\n");
        mpz_clear(total_comb);
        return NULL;
    }
    mpz_init_set(comb_array->num_comb, total_comb);
    comb_array->num_elem = num;
    comb_array->samples = sample_list;

    mpz_clear(total_comb);
    return comb_array;
}


//Compute the i-th combination on-the-fly using mixed-radix — no precomputed storage.
//index is an mpz_t, so indices beyond SIZE_MAX are supported.
//Returns a freshly-allocated array of num_elem pointers (caller frees via free_ith_combination).
void **get_the_ith_combination(combination_array *combinations, mpz_srcptr index)
{
    if(combinations == NULL) return NULL;
    // Bounds check: index must be < num_comb (num_comb is always >= 1 here)
    if(mpz_sgn(index) < 0) return NULL;
    if(mpz_cmp(index, combinations->num_comb) >= 0) return NULL;

    size_t num = combinations->num_elem;
    void **result = malloc(num * sizeof(void *));
    if (result == NULL) {
        fprintf(stderr, "Error: failed to allocate combination result array.\n");
        return NULL;
    }

    // Mixed-radix decomposition using GMP arbitrary-precision arithmetic
    mpz_t remaining, length_mpz, idx_mpz;
    mpz_init_set(remaining, index);
    mpz_init(length_mpz);
    mpz_init(idx_mpz);

    for(size_t j = 0; j < num; j++){
        size_t length = combinations->samples->samples[j].size;
        size_t elem_size = combinations->samples->samples[j].elem_size;

        // Convert length to mpz_t portably
        mpz_import(length_mpz, 1, -1, sizeof(size_t), 0, 0, &length);

        // idx = remaining % length;  remaining = remaining / length
        mpz_tdiv_qr(remaining, idx_mpz, remaining, length_mpz);

        // Convert idx back to size_t (idx < length ≤ SIZE_MAX, so it always fits)
        size_t idx = 0;
        mpz_export(&idx, NULL, -1, sizeof(size_t), 0, 0, idx_mpz);
        if(elem_size != 0 && idx > SIZE_MAX / elem_size) {
            free(result);
            mpz_clear(remaining);
            mpz_clear(length_mpz);
            mpz_clear(idx_mpz);
            return NULL;
        }

        result[j] = (void *)((char *)combinations->samples->samples[j].array + idx * elem_size);
    }

    mpz_clear(remaining);
    mpz_clear(length_mpz);
    mpz_clear(idx_mpz);

    return result;
}

//Helper function to free the memory from combination_array
void free_combination_array(combination_array *comb_array)
{
    if(comb_array == NULL) return;
    mpz_clear(comb_array->num_comb);
    free(comb_array);
}

//Helper function to free memory of the ith combination
void free_ith_combination(void **combination)
{
    if(combination == NULL) return;
    free(combination);
}

//Helper function to help free the memory from sample_array_list
void free_sample_array_list(sample_array_list *sample_list)
{
    if(sample_list == NULL) return;
    free(sample_list->samples);
    free(sample_list);
}

//Helper function to help free the memory from sample_array_list and also the array passed to it.
void free_sample_array_list_content(sample_array_list *sample_list)
{
    if(sample_list == NULL) return;
    for(size_t i = 0; i < sample_list->num_arrays; i++){
        free(sample_list->samples[i].array);
    }
    free(sample_list->samples);
    free(sample_list);
}
