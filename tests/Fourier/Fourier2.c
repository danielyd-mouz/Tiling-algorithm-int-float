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

static void swap_double(double *a, double *b)
{
    double tmp = *a;
    *a = *b;
    *b = tmp;
}

static void bit_reverse(double real[FOURIER_N], double imag[FOURIER_N])
{
    size_t j = 0;

    for(size_t i = 1; i < FOURIER_N; i++){
        size_t bit = FOURIER_N >> 1;
        while((j & bit) != 0){
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;

        if(i < j){
            swap_double(&real[i], &real[j]);
            swap_double(&imag[i], &imag[j]);
        }
    }
}

static void fft_forward(double real[FOURIER_N], double imag[FOURIER_N])
{
    bit_reverse(real, imag);

    for(size_t len = 2; len <= FOURIER_N; len <<= 1){
        double angle = -2.0 * FOURIER_PI / (double)len;
        double wlen_real = cos(angle);
        double wlen_imag = sin(angle);

        for(size_t start = 0; start < FOURIER_N; start += len){
            double w_real = 1.0;
            double w_imag = 0.0;

            for(size_t j = 0; j < len / 2; j++){
                size_t even = start + j;
                size_t odd = even + len / 2;

                double odd_real = real[odd] * w_real - imag[odd] * w_imag;
                double odd_imag = real[odd] * w_imag + imag[odd] * w_real;
                double even_real = real[even];
                double even_imag = imag[even];

                real[even] = even_real + odd_real;
                imag[even] = even_imag + odd_imag;
                real[odd] = even_real - odd_real;
                imag[odd] = even_imag - odd_imag;

                double next_w_real = w_real * wlen_real - w_imag * wlen_imag;
                double next_w_imag = w_real * wlen_imag + w_imag * wlen_real;
                w_real = next_w_real;
                w_imag = next_w_imag;
            }
        }
    }
}

// Radix-2 Cooley-Tukey FFT.
// Requires num_inputs = 8.
// Computes the real part of X[1] for the same forward transform as Fourier1.c.
int fourier(void **inputs, size_t num_inputs, void *output){
    if(num_inputs != FOURIER_N) return -1;

    double real[FOURIER_N];
    double imag[FOURIER_N];

    for(size_t i = 0; i < FOURIER_N; i++){
        real[i] = *((double *)inputs[i]);
        imag[i] = 0.0;
    }

    fft_forward(real, imag);
    *((double *)output) = real[FOURIER_K];
    return 0;
}
