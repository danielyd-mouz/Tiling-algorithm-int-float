#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <gmp.h>

int expo(void **inputs, size_t num_inputs, void *output){
    if (num_inputs != 2) return -1;

    uint64_t b = *((uint64_t *)inputs[0]);
    uint64_t e = *((uint64_t *)inputs[1]);
    mpz_ptr out = (mpz_ptr)output;

    mpz_t result;
    mpz_t base;
    mpz_init_set_ui(base, b);
    mpz_init_set_ui(result, 1);

    while(e > 0){
        if(e & 1){
            mpz_mul(result, result, base);
        }
        mpz_mul(base, base, base);
        e >>= 1;
    }

    mpz_set(out, result);
    mpz_clear(result);
    mpz_clear(base);
    return 0;
}