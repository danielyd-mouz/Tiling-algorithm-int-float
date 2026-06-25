#ifndef COMPARE_API_H
#define COMPARE_API_H

#include <stddef.h>

#ifdef _WIN32
#define COMPARE_EXPORT __declspec(dllexport)
#else
#define COMPARE_EXPORT __attribute__((visibility("default")))
#endif

/*
 * User DLL functions may use any exported name, but must use this signature.
 * Return 0 on success and write the numeric result to output.
 *
 * inputs points to the current sampled values and must not be modified.
 * output points to the declared output type. mpz_t and mpfr_t outputs are
 * already initialized by the comparison program.
 */
typedef int (*compare_target_fn)(
    void **inputs,
    size_t num_inputs,
    void *output
);

#endif
