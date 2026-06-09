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
#include "../header/type.h"
#include "../header/Tiling_function.h"
#include "../header/array_combination.h"

//new type that contains both the size of the sample array and also the array itself
typedef struct {
    size_t size;
    size_t elem_size;
    void *array;
} sample_array;

typedef struct {
    size_t num_arrays;
    size_t counter;
    sample_array *samples;
} sample_array_list;

//new type that contains the number of combinations, the number of elements in each combination, and a pointer to the 1-D array of pointers to the values in the Cartesian product of the input arrays
typedef struct {
    size_t num_comb;
    size_t num_elem;
    void **combinations;
} combination_array;

//Helper function to create a sample_array_list struct
sample_array_list *create_sample_array_list(size_t num_arrays)
{
    sample_array_list *sample_list = xmalloc(sizeof(sample_array_list));
    sample_list->num_arrays = num_arrays;
    sample_list->counter = 0;
    sample_list->samples = xmalloc(sizeof(sample_array) * num_arrays);
    return sample_list;
}

//Helper function to check if the required size of the output array exceeds the maximum allowed size
bool check_requiredSize(size_t num, size_t total_comb)
{
    mpz_t total_comb_mpz;
    mpz_init_set_ui(total_comb_mpz, total_comb);
    mpz_t num_mpz;
    mpz_init_set_ui(num_mpz, num);
    size_t void_size = sizeof(void *);
    mpz_t void_size_mpz;
    mpz_init_set_ui(void_size_mpz, void_size);
    mpz_t max_size;
    mpz_init_set_ui(max_size, SIZE_MAX);
    mpz_t required_size;
    mpz_init(required_size);
    mpz_mul(required_size, total_comb_mpz, void_size_mpz);
    mpz_mul(required_size, required_size, num_mpz);
    if(mpz_cmp(required_size, max_size) > 0){
        printf("The total size of the output array exceeds the maximum allowed.\n");
        mpz_clear(total_comb_mpz);
        mpz_clear(num_mpz);
        mpz_clear(void_size_mpz);
        mpz_clear(max_size);
        mpz_clear(required_size);
        return false;
    }
    mpz_clear(total_comb_mpz);
    mpz_clear(num_mpz);
    mpz_clear(void_size_mpz);
    mpz_clear(max_size);
    mpz_clear(required_size);
    return true;
}

//Helper function to put and array as a sample_array struct
void add_to_sample_array(sample_array_list *sample_list, size_t elem_size, size_t size, void *array)
{
    if(sample_list->counter == sample_list->num_arrays){
        return;
    }
    sample_array sample;
    sample.size = size;
    sample.elem_size = elem_size;
    sample.array = array;
    sample_list->samples[sample_list->counter] = sample;
    sample_list->counter++;
}

/*The function that get all the Cartisian product of the input arrays and return a 1-D array of pointers to the values 
Input:
    - sample_array_list *sample_list: a pointer to the struct of type sample_array_list, which contains a list of sample_array structs, and the number of arrays. Each sample_array struct contains the size of the array, the size of each element in the array, and a pointer to the array itself. The arrays in the sample_array struct can be of different types and sizes.
Output:
    - combination_array *: a struct containing the number of combinations, the number of elements in each combination, and a pointer to the 1-D array of pointers to the values in the Cartesian product of the input arrays. The caller is responsible for freeing the memory allocated for the output array.
*/
combination_array *combine_arrays(sample_array_list *sample_list)
{
    size_t num = sample_list->num_arrays;
    if(num == 0) return NULL;

    size_t total_comb = 1;
    for(size_t i = 0; i<sample_list->num_arrays; i++){
        total_comb = total_comb * sample_list->samples[i].size;
    }
    
    if(!check_requiredSize(num, total_comb)) return NULL;
    if(total_comb == 0) return NULL;

    void **result = xmalloc(sizeof(void *) * total_comb * num);
    for(size_t i = 0; i<total_comb; i++){
        size_t remaining = i;
        for(size_t j = 0; j<num; j++){
            size_t length = sample_list->samples[j].size;
            size_t elem_size = sample_list->samples[j].elem_size;
            size_t index = remaining % length;
            remaining = remaining / length;
            size_t result_index = i * num + j;
            result[result_index] = (void *)((char *)sample_list->samples[j].array + index * elem_size);
        }
    }
    combination_array *comb_array = xmalloc(sizeof(combination_array));
    comb_array->num_comb = total_comb;
    comb_array->num_elem = num;
    comb_array->combinations = result;

    return comb_array;
}


//Helper function to get the i-th combination from the output of combine_arrays function
void **get_the_ith_combination(combination_array *combinations, size_t index)
{
    if(combinations == NULL) return NULL;
    if(index >= combinations->num_comb) return NULL;
    
    void **result = xmalloc(combinations->num_elem * sizeof(void *));
    for(size_t i = 0; i<combinations->num_elem; i++){
        result[i] = combinations->combinations[index * combinations->num_elem + i];
    }
    return result;
}

void free_combination_array(combination_array *comb_array)
{
    if(comb_array == NULL) return;
    free(comb_array->combinations);
    free(comb_array);
}

void free_ith_combination(void **combination)
{
    if(combination == NULL) return;
    free(combination);
}

void free_sample_array_list(sample_array_list *sample_list)
{
    if(sample_list == NULL) return;
    for(size_t i = 0; i < sample_list->num_arrays; i++){
        free(sample_list->samples[i].array);
    }
    free(sample_list->samples);
    free(sample_list);
}