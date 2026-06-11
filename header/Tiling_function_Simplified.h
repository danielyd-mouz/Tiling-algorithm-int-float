#ifndef TILING_SIMPLIFIED_H
#define TILING_SIMPLIFIED_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/*
 Simplified version of the tiling function togenerate a tiling function for int type.
 Could be used without GMP library being installed

 Input Parameters:
    - num: the number of samples included in the returned array.

    - precision: the type of int the user is looking for. Power of two sizes between 8 and 64 bits.

    - sign: whether the int type is signed or not. true: signed, false: unsigned.

 Output:
    - a generic pointer containing the type of int the user is looking for
      - All signed int types will be returned as data structure unsigned_int containing an array of int64_t
        and all unsigned int types will be returned as data structure unsigned_int containing an array of uint64_t
      - The caller can use the implementation function provided to manipulate the output data
*/

void *tiling_int_simplified(size_t num, uint32_t precision, bool sign);
// REQUIRES: num > 0;
// REQUIRES: 0<precision && precision <= 256;
// REQUIRES: precision == 256 ||num <= (size_t)(1<<precision - 1);  ???
// ENSURES: result != NULL && \length(result) == num;

/*
 Simplified version of the tiling function to generate a tiling function for float type.
 Could be used without MPFR library being installed

 Input Parameters:
    - num: the number of samples included in the returned array.

    - precision: the type of float the user is looking for.
                 32->float, 64->double.

 Output:
    - the array with num samples of the designated type.
      - All float types will be returned as float or double.
      - The caller can cast the pointer to the correct type based on the input parameters.
*/
void *tiling_float_simplified(size_t num, uint32_t precision);
// REQUIRES: num > 0;
// REQUIRES: num <= SIZE_MAX/(precision/8);
// REQUIRES: precision == 32 || precision == 64;
// ENSURES: result != NULL && \length(result) == num;

#endif
