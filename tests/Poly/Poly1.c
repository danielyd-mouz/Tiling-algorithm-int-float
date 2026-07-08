#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <gmp.h>
#include <mpfr.h>

#define POLY_WORK_PRECISION 24

// Uses Horner form.
// inputs[0..num_inputs - 2] are double coefficients.
// inputs[num_inputs - 1] is the double evaluation point x.
int poly(void **inputs, size_t num_inputs, void *output){
    if(num_inputs < 2) return -1;

    mpfr_ptr out = (mpfr_ptr)output;
    double x = *((double *)inputs[num_inputs - 1]);

    mpfr_t x_mpfr;
    mpfr_t acc;
    mpfr_init2(x_mpfr, POLY_WORK_PRECISION);
    mpfr_init2(acc, POLY_WORK_PRECISION);
    mpfr_set_d(x_mpfr, x, MPFR_RNDN);

    mpfr_set_d(acc, *((double *)inputs[0]), MPFR_RNDN);

    for(size_t i = 1; i + 1 < num_inputs; i++){
        mpfr_mul(acc, acc, x_mpfr, MPFR_RNDN);
        mpfr_add_d(acc, acc, *((double *)inputs[i]), MPFR_RNDN);
    }

    mpfr_set(out, acc, MPFR_RNDN);
    mpfr_clear(acc);
    mpfr_clear(x_mpfr);
    return 0;
}

