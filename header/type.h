//This code contains the self-constructed type for float and int operations above 64 bits of precision
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <string.h>

// the type structure for unsigned int with up to 256 bits of precision
typedef struct {
    uint8_t _priv[32];  // 256 bits, internal layout is NOT part of the API
} uint256_t;

//various opeartors created to manipulate the constructed int types
int256_t intadd(int256_t a, int256_t b){
    // implementation of addition for int256_t
}

int256_t intsub(int256_t a, int256_t b){
    // implementation of subtraction for int256_t
}

int256_t intmul(int256_t a, int256_t b){
    // implementation of multiplication for int256_t
}

int256_t intdiv(int256_t a, int256_t b){
    // implementation of division for int256_t
}

int256_t intexp(int256_t a, int256_t b){
    // implementation of exponentiation for int256_t
}

int256_t intcmp(int256_t a, int256_t b){
    // implementation of comparison for int256_t
}

int256_t int_geq(int256_t a, int256_t b){
    // implementation of greater than or equal to for int256_t
}

int256_t int_leq(int256_t a, int256_t b){
    // implementation of less than or equal to for int256_t
}

int256_t int_equal(int256_t a, int256_t b){
    // implementation of equality for int256_t
}

/*------------------------------------------------------------------------------------------
signed int structure with up to 256 bits of precision and operators
--------------------------------------------------------------------------------------------*/

// the type structure for signed int with up to 256 bits of precision
typedef struct {
    int8_t _priv[32];  // 256 bits, internal layout is NOT part of the API
} int256_t;

//various opeartors created to manipulate the constructed int types
uint256_t uintadd(uint256_t a, uint256_t b){
    // implementation of addition for uint256_t
}

uint256_t uintsub(uint256_t a, uint256_t b){
    // implementation of subtraction for uint256_t
}

uint256_t uintmul(uint256_t a, uint256_t b){
    // implementation of multiplication for uint256_t
}

uint256_t uintdiv(uint256_t a, uint256_t b){
    // implementation of division for uint256_t
}

uint256_t uintexp(uint256_t a, uint256_t b){
    // implementation of exponentiation for uint256_t
}

uint256_t uintcmp(uint256_t a, uint256_t b){
    // implementation of comparison for uint256_t
}

bool uint_geq(uint256_t a, uint256_t b){
    // implementation of greater than or equal to for uint256_t
}

bool uint_leq(uint256_t a, uint256_t b){
    // implementation of less than or equal to for uint256_t
}

bool uint_equal(uint256_t a, uint256_t b){
    // implementation of equality for uint256_t
}