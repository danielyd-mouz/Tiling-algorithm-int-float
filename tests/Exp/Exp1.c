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
    if(num_inputs != 2) return -1;
    uint64_t b = *((uint64_t *)inputs[0]);
    uint64_t e = *((uint64_t *)inputs[1]);
    mpz_ptr out = (mpz_ptr)output;
    mpz_ui_pow_ui(out,b,e);
    return 0;
}