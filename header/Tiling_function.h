#ifndef TILING_H
#define TILING_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <gmp.h>
#include <mpfr.h>

// Enum type to represent the round mode for the float tiling function
typedef enum{
    RNDN,
    RNDZ,
    RNDU,
    RNDD,
    RNDA
}rnd_type;

/*
The tiling function to generate a 1-D array of int samples with precision between 1 and 256 of type mpz_t.

Input Parameters:
   - arr: an uninitialized mpz_t array that the user want the samples to be stored in.

   - num: the number of samples included in the returned array.

   - precision: the number of bits the user wants their samples to have. Between 1 and 256. The program might still be able to handle the case if precision exceeds this range, but generally not recommended.

   - sign: whethe the resulting sample is signed or not. true: signed, false: unsigned.

Output:
   No output is returned, but the input array will be filled with num samples of the designated type.
   The caller can use the operations provided by the GMP library to manipulate the output data.
*/
void tiling_int(mpz_t **arr, size_t num, uint32_t precision, bool sign);
// REQUIRES: num > 0;
// REQUIRES: 0<precision && precision <= 256;


//This is just a helper function to clear the mpz_t array created by tiling_int, which is not automatically cleared by the caller.
/*the function will only clear the inner array, and left the outer pointer to the user */
void clear_iarray(mpz_t **array, size_t num);
// REQUIRES: array != NULL;
// REQUIRES: num = \length(array);


/*
The tiling function to generate a 1-D array of float samples with precision between 1 and 256 of type mpfr_t.

Input Parameters:
   - arr: an uninitialized mpfr_t array that the user want the samples to be stored in.

   - num: the number of samples included in the returned array.

   - precision: The number of significand the user want their samples to have.
      bewteen 1 and 256. The program might still be able to handle the case if precision exceeds this range, but generally not recommended.

   - mantissa: The number of bits in the exponent part of the float. Between 1 and 256. The program might still be able to handle the case if precision exceeds this range, but generally not recommended.

       For example, for a float32, the precision is 32 and the mantissa is 24. For a float64, the precision is 64 and the mantissa is 53.
   - round: Enum type rnd_type with values corresponding to different rounding mode to use for the floating-point operations, rounding to nearest if left unspecified.
      // MPFR_RNDN  round to nearest(default)
      // MPFR_RNDZ  round toward zero
      // MPFR_RNDU  round toward +infinity
      // MPFR_RNDD  round toward -infinity
      // MPFR_RNDA  round away from zero

Output:
   No output is returned, but the input array will be filled with num samples of the designated type.
   The caller can use the operations provided by the MPFR library to manipulate the output data.
*/
void tiling_float(mpfr_t **arr, size_t num, mpfr_prec_t precision, mpfr_prec_t mantissa, rnd_type round);
// REQUIRES: num > 0;
// REQUIRES: num <= SIZE_MAX/sizeof(mpfr_t);
// REQUIRES: precision >= 0 && precision <= 256;
// ENSURES: result != NULL && \length(result) == num;

//This is just a helper function to clear the mpfr_t array created by tiling_float, which is not automatically cleared by the caller.
/*the function will only clear the inner array, and left the outer pointer to the user */
void clear_farray(mpfr_t **array, size_t num);
// REQUIRES: array != NULL;

#endif