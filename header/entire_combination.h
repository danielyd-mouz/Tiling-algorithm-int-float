#ifndef ENTIRE_COMBINATION_H
#define ENTIRE_COMBINATION_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <gmp.h>

typedef struct sample_array_list sample_array_list;
typedef struct combination_array combination_array;

//Helper function to create a pointer to a sample_array_list
sample_array_list *create_sample_array_list(size_t num_arrays);

//Helper function to put an array as a sample_array struct into the sample_array_list pointer
void add_to_sample_array(sample_array_list *sample_list, size_t elem_size, size_t size, void *array);

//Lightweight "constructor" — stores the input arrays and the total number of combinations
//(as an arbitrary-precision mpz_t). No combinations are materialized.
combination_array *combine_arrays(sample_array_list *sample_list);

//Compute the i-th combination on-the-fly via mixed-radix. index is an arbitrary-precision
//mpz_t, so indices beyond SIZE_MAX are supported. Returns a freshly-allocated array of
//void * pointers; caller frees via free_ith_combination.
void **get_the_ith_combination(combination_array *combinations, mpz_srcptr index);

//Helper function to free the memory allocated for the output of combine_arrays function
void free_combination_array(combination_array *comb_array);

//Helper function to free the memory allocated for the i-th combination from the output of combine_arrays function
void free_ith_combination(void **combination);

//Helper function to free the memory allocated for the sample_array_list struct
void free_sample_array_list(sample_array_list *sample_list);

//Helper function to free the memory allocated for the sample_array_list and also all the arrays passed to it
void free_sample_array_list_content(sample_array_list *sample_list);

#endif
