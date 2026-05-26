//This code contains the self-constructed type for float and int operations above 64 bits of precision
#include "type.h"
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


/*------------------------------------------------------------------------------------------
signed int structure with up to 256 bits of precision and operators
--------------------------------------------------------------------------------------------*/

// the type structure for signed int with up to 256 bits of precision
typedef struct {
    int8_t _priv[32];  // 256 bits, internal layout is NOT part of the API
} int256_t;

//various opeartors created to manipulate the constructed int types
