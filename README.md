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


## Function specification:

### Int Tiling: 
ints supports precision, aka power of 2, less than or equal to 256.
-If MPFR library is installed:
    float functions support arbitrary precision up to 256 bits and arbitrary mantissa length.
 Else:
    float functions with types float32 or float64

The int structure is also provided to users for purpose of creating structure, but other operators are provided to users and are kept opaque.

Int inputs:
- signed : bool
- precision_bits : uint32_t
- num_samples : size_t

Outputs:

Divide into signed and unsigned cases 
- Signed cases all have int64_t 
- Unsigned cases all have uint64_t 

The sequence of samples being added: special boundary cases -> power of 2 and complements -> evenly spaced values -> values starting from min incrementing by 1 each time


### Float Tiling:
-If MPFR library is installed:
    float functions support arbitrary precision up to 256 bits and arbitrary mantissa length.
 Else:
    float functions with types float32 or float64

#### MPFR Installed and Run
Float inputs: 
- precision_bits : uint32_t (up to 256 bits)
- num_samples : size_t

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