# Tiling-algorithm-int-float
## Introduction
This project intends to develop a smart way of tiling given certain variable type, amount of outputs and range.

User must have a c environment equipped in their editor to access the code

The additional GMP and MPFR libraries are required to use int and float tiling function with arbitrary size put to 256 bits and mantissa size. If the GMP and MPFS libraries are not installed, user can still use the simplifid tiling function for int with precision bits up to 64 and float that supports c's original float32 and float64

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
   - *rnd: Pointer of type mpfr_rnd_t * pointing to the rounding mode to use for the floating-point operations, rounding to nearest if left unspecified.
      // MPFR_RNDN  round to nearest(default)
      // MPFR_RNDZ  round toward zero
      // MPFR_RNDU  round toward +infinity
      // MPFR_RNDD  round toward -infinity
      // MPFR_RNDA  round away from zero

Output:
   No output is returned, but the input array will be filled with num samples of the designated type.
   The caller can use the operations provided by the MPFR library to manipulate the output data.

#### Run without MPFR
Divide into float and double cases
- float cases with precision of 32 bits
- double cases with precision of 64 bits

The sequence of samples being added: special boundary cases -> power of 2 and complements -> evenly spaced values -> values starting from min incrementing by 1 each time

## Test(not implemented yet)
compile and run test:
-make test

delete build directory:
-make clean