#ifndef TILING_H
#define TILING_H

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
#include <mpfr.h>
#include "../header/type.h"
#include "../header/Tiling_function.h"



void *tiling_float(size_t num, uint32_t precision);
// REQUIRES: num > 0;
// REQUIRES: num <= SIZE_MAX/(precision/8);   ???
// REQUIRES: precision >= 0 && precision <= 256;
// ENSURES: result != NULL && \length(result) == num;




#endif