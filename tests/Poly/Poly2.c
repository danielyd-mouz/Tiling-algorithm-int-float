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

// Uses expanded form.
// inputs[0..num_inputs - 2] are double coefficients.
// inputs[num_inputs - 1] is the double evaluation point x.
int poly(void **inputs, size_t num_inputs, void *output){
    if(num_inputs < 2) return -1;

    mpfr_ptr out = (mpfr_ptr)output;
    double x = *((double *)inputs[num_inputs - 1]);
    size_t coeff_count = num_inputs - 1;

    mpfr_t x_mpfr;
    mpfr_t sum;
    mpfr_t term;
    mpfr_t power;
    mpfr_init2(x_mpfr, POLY_WORK_PRECISION);
    mpfr_init2(sum, POLY_WORK_PRECISION);
    mpfr_init2(term, POLY_WORK_PRECISION);
    mpfr_init2(power, POLY_WORK_PRECISION);

    mpfr_set_d(x_mpfr, x, MPFR_RNDN);
    mpfr_set_zero(sum, 0);

    for(size_t i = 0; i < coeff_count; i++){
        size_t exponent = coeff_count - 1 - i;

        mpfr_set_ui(power, 1, MPFR_RNDN);
        for(size_t j = 0; j < exponent; j++){
            mpfr_mul(power, power, x_mpfr, MPFR_RNDN);
        }

        mpfr_set_d(term, *((double *)inputs[i]), MPFR_RNDN);
        mpfr_mul(term, term, power, MPFR_RNDN);
        mpfr_add(sum, sum, term, MPFR_RNDN);
    }

    mpfr_set(out, sum, MPFR_RNDN);
    mpfr_clear(power);
    mpfr_clear(term);
    mpfr_clear(sum);
    mpfr_clear(x_mpfr);
    return 0;
}
