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

typedef struct sample_array_list;
typedef struct combination_array;

//Helper function to create a pointer to a sample_array_list
sample_array_list *create_sample_array_list(size_t num_arrays);

//Helper function to put an array as a sample_array struct into the sample_array_list pointer
void add_to_sample_array(sample_array_list *sample_list, size_t elem_size, size_t size, void *array);

/*The function that get all the Cartisian product of the input arrays and return a 1-D array of pointers to the values 
Input:
    - sample_array_list *sample_list: a pointer to the struct of type sample_array_list, which contains a list of sample_array structs, and the number of arrays. Each sample_array struct contains the size of the array, the size of each element in the array, and a pointer to the array itself. The arrays in the sample_array struct can be of different types and sizes.
Output:
    - combination_array *: a struct containing the number of combinations, the number of elements in each combination, and a pointer to the 1-D array of pointers to the values in the Cartesian product of the input arrays. The caller is responsible for freeing the memory allocated for the output array.
*/
combination_array *combine_arrays(sample_array_list *sample_list);

//Helper function to get the i-th combination from the output of combine_arrays function
void **get_the_ith_combination(combination_array *combinations, size_t index);

//Helper function to free the memory allocated for the output of combine_arrays function
void free_combination_array(combination_array *comb_array);

//Helper function to free the memory allocated for the i-th combination from the output of combine_arrays function
void free_ith_combination(void **combination);

//Helper function to free the memory allocated for the sample_array_list struct
void free_sample_array_list(sample_array_list *sample_list);