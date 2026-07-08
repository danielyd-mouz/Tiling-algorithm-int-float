#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <string.h>

#define FOURIER_N 8
#define FOURIER_K 1
#define FOURIER_PI 3.141592653589793238462643383279502884

// Naive DFT reference.
// Requires num_inputs = 8.
// Computes the real part of X[1] for the forward transform:
// X[k] = sum_n x[n] * exp(-2*pi*i*k*n/N)
int fourier(void **inputs, size_t num_inputs, void *output){
    if(num_inputs != FOURIER_N) return -1;

    double real = 0.0;
    for(size_t n = 0; n < FOURIER_N; n++){
        double x = *((double *)inputs[n]);
        double angle = -2.0 * FOURIER_PI * FOURIER_K * (double)n / FOURIER_N;
        real += x * cos(angle);
    }

    *((double *)output) = real;
    return 0;
}
