#ifndef TILING_H
#define TILING_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <float.h>
#include <limits.h>
#include <math.h>

/*
 generate a tiling function for int type.

 Input Parameters:
    - num: the number of samples included in the returned array.

    - precision: the type of int the user is looking for. Power of two sizes between 8 and 64 bits.

    - sign: whether the int type is signed or not. true: signed, false: unsigned.

 Output:
    - a generic pointer containing the type of int the user is looking for.
*/
void *tiling_int(size_t num, uint32_t precision, bool sign);
// REQUIRES: num > 0;
// REQUIRES: 0<precision && precision <= 64;
// REQUIRES: precision == 64 ||num <= (size_t)(1<<precision - 1);
// ENSURES: result != NULL && \length(result) == num;

/*
 generate a tiling function for float type.

 Input Parameters:
    - num: the number of samples included in the returned array.

    - precision: the type of float the user is looking for. 
                 32->float, 64->double.

 Output:
    - the array with num samples of the designated type.
*/
void *tiling_float(size_t num, uint32_t precision);
// REQUIRES: num > 0;
// REQUIRES: num <= SIZE_MAX/(precision/8);
// REQUIRES: precision == 32 || precision == 64;
// ENSURES: result != NULL && \length(result) == num;

#endif
