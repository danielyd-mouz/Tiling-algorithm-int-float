#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <float.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
/*#include "../header/type.h"
typedef struct {
   int8_t _priv[32];  // 256 bits, internal layout is NOT part of the API
} int256_t;

int256_t intadd(int256_t a, int256_t b); // return a+b, if the result exceeds the maximum value of the type, the behavior is undefined
int256_t intsub(int256_t a, int256_t b); // return a-b, if the result is less than the minimum value of the type, the behavior is undefined
int256_t intmul(int256_t a, int256_t b); // return a*b, if the result exceeds the maximum value of the type, the behavior is undefined
int256_t intdiv(int256_t a, int256_t b); // return a/b, if b == 0, the behavior is undefined
int256_t intexp(int256_t a, int256_t b); // return a^b
int256_t intcmp(int256_t a, int256_t b); // return -1 if a < b, 0 if a == b, 1 if a > b
bool int_geq(int256_t a, int256_t b);    // return true if a >= b, false otherwise
bool int_leq(int256_t a, int256_t b);    // return true if a <= b, false otherwise
bool int_equal(int256_t a, int256_t b);  // return true if a == b, false otherwise



typedef struct {
   uint8_t _priv[32];  // 256 bits, internal layout is NOT part of the API
} uint256_t;

uint256_t uintadd(uint256_t a, uint256_t b); // return a+b, if the result exceeds the maximum value of the type, the behavior is undefined
uint256_t uintsub(uint256_t a, uint256_t b); // return a-b, if a < b, the behavior is undefined
uint256_t uintmul(uint256_t a, uint256_t b); // return a*b, if the result exceeds the maximum value of the type, the behavior is undefined
uint256_t uintdiv(uint256_t a, uint256_t b); // return a/b, if b == 0, the behavior is undefined
uint256_t uintexp(uint256_t a, uint256_t b); // return a^b
uint256_t uintcmp(uint256_t a, uint256_t b); // return -1 if a < b, 0 if a == b, 1 if a > b
bool uint_geq(uint256_t a, uint256_t b);     // return true if a >= b, false otherwise
bool uint_leq(uint256_t a, uint256_t b);     // return true if a <= b, false otherwise
bool uint_equal(uint256_t a, uint256_t b);   // return true if a == b, false otherwise   */

