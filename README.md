# Tiling-algorithm-int-float
## Introduction
This project intends to develop a smart way of tiling given certain variable type, amount of outputs and range, and includes a helper function to produce Cartitian products of the array provided by users

User must have a c environment equipped in their editor to access the code

The additional GMP and MPFR libraries are required to use int and float tiling function with arbitrary size put to 256 bits and mantissa size. If the GMP and MPFS libraries are not installed, user can still use the simplifid tiling function for int with precision bits up to 64 and float that supports c's original float32 and float64

## Directory:
├── code/           # Implementation files
│   ├── Tiling_function.c          (GMP/MPFR tiling)
│   ├── Tiling_function_Simplified.c  (no external deps)
│   ├── type.c                     (256-bit custom types)
│   └── entire_combination.c       (Cartesian product)
├── header/         # Public headers
│   ├── Tiling_function.h
│   ├── Tiling_function_Simplified.h
│   ├── type.h
│   └── entire_combination.h
├── tests/          # Test suite
│   └── test_Tiling_function.c
├── Makefile
└── README.md

## Dependencies

This project depends on the following external libraries:

- GMP (GNU Multiple Precision Arithmetic Library)
- MPFR (GNU Multiple Precision Floating-Point Reliable Library)

This repository does not include the source code or binary files of GMP or MPFR. Users should install these libraries separately on their own system.

GMP is distributed under the GNU LGPLv3 and GNU GPLv2 licenses. MPFR is distributed under the GNU LGPLv3-or-later license. Please refer to the official GMP and MPFR documentation for their license terms.

More details can be found at website: 
https://gmplib.org/
https://www.mpfr.org/

**IMPORTANT: The MPFR library relies on GMP library, even if users just want to use float tiling function with arbitrary precision, they should still introduce GMP library to support MPFR library.

### Instillation:
Linux (Debian/Ubuntu): sudo apt install libgmp-dev libmpfr-dev
macOS: brew install gmp mpfr
Windows (MSYS2): pacman -S mingw-w64-x86_64-gmp mingw-w64-x86_64-mpfr

### Benefits of using GNU GMP and MPFR library:
    -Their libraries, behaviors are well-tested and can achieve much more than implementing a new type myself
    -The two datatypes mpz_t for signed int in GMP and mpfr_t for arbitrary precision float in MPFR can convert to each other, which can be useful when doing tiling
    -The mpfr_t type even supports different rounding behaviors and changing precision of a variable after the variable is initialized

### Special restrictions made here
    -Since the mpz_t library is designed in a way that the sign is not stored in the highest bit, but in a seperate variable, the signed integer range is larger than in c. For exmaple, a 256 bit signed int, in original c enviroment, can represent only -2^255 to 2^255-1 due to the occupation of sign bit in the highest bit. However, under mpz_t, the integer value can be represented from -2^256 to 2^256. NEVERTHELESS, to refer back to and mimic orignal c envirnment, this project will restrict the range of value representable of an n bit precision int to -2^(n-1) to 2^(n-1)-1

## Function specification:

### Int Tiling: 
ints supports precision, aka power of 2, less than or equal to 256.
-If GMP library is installed:
    float functions support arbitrary precision up to 256 bits and arbitrary mantissa length.
 Else:
    float functions with types float32 or float64

The int structure is also provided to users for purpose of creating structure, but other operators are provided to users and are kept opaque.

#### GMP Installed and Run
Input Parameters:
   - arr: an uninitialized mpz_t array that the user want the samples to be stored in.

   - num: the number of samples included in the returned array.

   - precision: the number of bits the user wants their samples to have. Between 1 and 256. The program might still be able to handle the case if precision exceeds this range, but generally not recommended.

   - sign: whethe the resulting sample is signed or not. true: signed, false: unsigned.

Output:
   No output is returned, but the input array will be filled with num samples of the designated type.
   The caller can use the operations provided by the GMP library to manipulate the output data.

The sequence of samples being added: special boundary cases -> power of 2 and complements -> evenly spaced values -> values starting from min incrementing by 1 each time

#### Run Without GMP Library
Divide into Signed and unsigned cases, both supporting up to 64 bits precision


### Float Tiling:
-If GMP and MPFR libraries are installed:
    float functions support arbitrary precision up to 256 bits and arbitrary mantissa length.
 Else:
    float functions with types float32 or float64

#### GMP and MPFR Installed and Run
Input Parameters:
   - arr: an uninitialized mpfr_t array that the user want the samples to be stored in.

   - num: the number of samples included in the returned array.

   - precision: The number of significand the user want their samples to have.
      bewteen 1 and 256. The program might still be able to handle the case if precision exceeds this range, but generally not recommended.

   - mantissa: The number of bits in the exponent part of the float. Between 1 and 256. The program might still be able to handle the case if precision exceeds this range, but generally not recommended.

       For example, for a float32, the precision is 32 and the mantissa is 24. For a float64, the precision is 64 and the mantissa is 53.
   - round: An enum value that represents the five possible rounding modes in MPFR library, rounding to nearest if left unspecified.
      // RNDN  round to nearest(default)
      // RNDZ  round toward zero
      // RNDU  round toward +infinity
      // RNDD  round toward -infinity
      // RNDA  round away from zero

Output:
   No output is returned, but the input array will be filled with num samples of the designated type.
   The caller can use the operations provided by the MPFR library to manipulate the output data.

#### Run without MPFR
Divide into float and double cases
- float cases with precision of 32 bits
- double cases with precision of 64 bits

The sequence of samples being added: special boundary cases -> power of 2 and complements -> evenly spaced values -> values starting from min incrementing by 1 each time

## Combination Function
This project are also equiped with a function that can do the Cartesian products of multiple arrays of arbitrary type.
Since there can be extremely easy for the numbre of combinations to exceed the limit of size_t, and requires a tons of spaces to store such many combinations, I implement the algorith using mixed-radix approach to calculate the index of each element in each array without taking up any extra spaces. However, since samples in different arrays many have different types, we put all values in a void pointer, where users later need to memorize the type of each position. Since the combination array is produced according to the sequency of arrays being put into the sample_array_list, the order of types is exactly the same as the order that users put arrays in sample_array_list.

### Prerequisite 
A c environment
In order to use the function from this part of the project, GMP environment is required

### Specification
The projects introduces several new datatypes, which are all kept invisible to clients:
- sample_array       : The datatype that stores an individual array, including its element size, number of elements and array itself
- sample_array_list  : The datatype that stores a list of arrays to do the Cartesian products, recording number of arrays and list of sample_array
- combination_array  : The datatype that store parameters related to the combination

And functions that allow users to interact with the datatypes in certain ways:
- sample_array_list *create_sample_array_list(size_t num_arrays);
   //creates an empty sample array list

- void add_to_sample_array(sample_array_list *sample_list, size_t elem_size, size_t size, void *array);
   //This function add an array casted to void * with arbitrary size and element type into a sample_array_list

- combination_array *combine_arrays(sample_array_list *sample_list);
   //This function generate a combination_array out of the provided sample_array_list

- void **get_the_ith_combination(combination_array *combinations, mpz_srcptr index);
   //This function can get the indexth combination in the form of a one-d array containning void *to one values from each array

- void free_combination_array(combination_array *comb_array);
   //This function frees the memory allocated by took by combination_array *comb_array

- void free_ith_combination(void **combination)
   //This function clears the space of void **combination

- void free_sample_array_list(sample_array_list *sample_list);
   //This function frees the memory allocated when initializing sample_array_list *sample_list

- void free_sample_array_list_content(sample_array_list *sample_list);
   //This function frees the memory of sample_list AND ALSO calls free() on every array passed to it via add_to_sample_array
   //Use only when you want the library to take full ownership of the arrays. Do NOT call both free_sample_array_list and this function — pick one.

## Memory Management

This library allocates memory internally in several places. The following rules describe who owns each allocation and how to free it correctly.

### Tiling functions (`tiling_int` / `tiling_float`)

- Both functions **allocate the output array for you** — the `arr` parameter must point to an uninitialized `mpz_t *` (or `mpfr_t *`), and the function will set it to a freshly-allocated array.
- **Do NOT use plain `free()`** on the result — the array contains GMP/MPFR objects that must be torn down individually first.
- Use the provided cleanup functions instead:

| Function | What it frees |
|---|---|
| `clear_iarray(mpz_t **array, size_t num)` | Calls `mpz_clear` on every element, then `free` on the outer array |
| `clear_farray(mpfr_t **array, size_t num)` | Calls `mpfr_clear` on every element, then `free` on the outer array |

```c
mpz_t *samples = NULL;
tiling_int(&samples, 100, 64, true);
// ... use samples ...
clear_iarray(&samples, 100);   // correct — frees inner mpz_t objects and the array
// free(samples);              // WRONG — leaks internal mpz_t resources
```

### MPFR global cleanup

MPFR maintains internal caches and a memory pool. After **all** MPFR operations are finished (typically at the end of `main`), you must call:

```c
mpfr_free_cache();
mpfr_free_pool();
```

These are required by the MPFR library — skipping them will leak memory. This applies even if you already called `clear_farray` on every array.

### Combination function — ownership and destruction order

#### Who owns the arrays?

When you call `add_to_sample_array`, the library stores a **reference** to your array. You have two choices for cleanup:

| Function | Behavior |
|---|---|
| `free_sample_array_list(sample_list)` | Frees the list structure **only**. You remain responsible for freeing the arrays you passed in. |
| `free_sample_array_list_content(sample_list)` | Frees the list structure **and** calls `free()` on every array you passed in. Use this when you want the library to take ownership of your arrays. |

**Choose one** — never call both, or the arrays will be double-freed.

#### Destruction order: free `combination_array` before `sample_array_list`

A `combination_array` holds a pointer to the `sample_array_list` that created it. If you free the `sample_array_list` first, any later call to `get_the_ith_combination` will dereference freed memory. The required teardown order is:

```c
// Correct order
free_combination_array(comb);                  // 1st — drops its reference to the list
free_sample_array_list(sample_list);           // 2nd — safe, nothing references it anymore
// OR
free_sample_array_list_content(sample_list);   // 2nd — also frees the underlying arrays
```

```c
// WRONG order — sample_list is freed while comb still references it
free_sample_array_list(sample_list);
free_combination_array(comb);                  // comb holds a dangling pointer to sample_list!
```

#### Per-combination results

`get_the_ith_combination` returns a freshly-allocated array of `void *` pointers. Free it with:

```c
free_ith_combination(combination);
```

This only frees the pointer array — the individual elements point into your original arrays and must not be freed separately.

### Summary of ownership

| You provide | Handed to | Freed by |
|---|---|---|
| `mpz_t *` / `mpfr_t *` (set to `NULL`) | `tiling_int` / `tiling_float` | `clear_iarray` / `clear_farray` |
| Your data arrays (via `malloc`) | `add_to_sample_array` | You, **or** `free_sample_array_list_content` — pick one |
| `sample_array_list *` | `create_sample_array_list` | `free_sample_array_list` (or `_content`) |
| `combination_array *` | `combine_arrays` | `free_combination_array` |
| `void **` (per-combination) | `get_the_ith_combination` | `free_ith_combination` |
| MPFR internal caches | MPFR library | `mpfr_free_cache()` + `mpfr_free_pool()` |
   
## Test(not implemented yet)
compile and run test:
-make test

delete build directory:
-make clean