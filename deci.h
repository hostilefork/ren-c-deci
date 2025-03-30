//
//  File: %deci.h
//  Summary: "Deci Datatype Function Prototypes"
//  Project: "Rebol 3 Interpreter and Run-time"
//  Author: "Ladislav Mecir"
//  Homepage: https://github.com/metaeducation/ren-c/
//
//=////////////////////////////////////////////////////////////////////////=//
//
// Copyright 2012 REBOL Technologies
// REBOL is a trademark of REBOL Technologies
//
// See README.md and CREDITS.md for more information.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//=////////////////////////////////////////////////////////////////////////=//
//
// Code from R3-Alpha for "deci", a type that was used to implement MONEY!.
//
// Ren-C takes a different strategy in its core, and makes MONEY! an immutable
// string type (like EMAIL! and URL!).  Instead of just deleting this code,
// it was moved to an extension... and DECI! is a datatype that you can
// optionally build into the executable (or load dynamically).
//
// See remarks in README.md for more information.
//

typedef struct {
    unsigned m0:32;  /* significand, lowest part */
    unsigned m1:32;  /* significand, continuation */
    unsigned m2:23;  /* significand, highest part */
    unsigned s:1;    /* sign, 0 means nonnegative, 1 means nonpositive */
    int e:8;         /* exponent */
} deci;


/* unary operators - logic */
bool deci_is_zero (const deci a);

/* unary operators - deci */
deci deci_abs (deci a);
deci deci_negate (deci a);

/* binary operators - logic */
bool deci_is_equal (deci a, deci b);
bool deci_is_lesser_or_equal (deci a, deci b);
bool deci_is_same (deci a, deci b);

/* binary operators - deci */
deci deci_add (deci a, deci b);
deci deci_subtract (deci a, deci b);
deci deci_multiply (const deci a, const deci b);
deci deci_divide (deci a, deci b);
deci deci_mod (deci a, deci b);

/* conversion to deci */
deci int_to_deci (int64_t a);
deci decimal_to_deci (double a);
deci string_to_deci (const Byte* s, const Byte* *endptr);
deci binary_to_deci(const Byte s[12]);

/* conversion to other datatypes */
int64_t deci_to_int (const deci a);
double deci_to_decimal (const deci a);
int32_t deci_to_string(Byte* string, const deci a, const Byte symbol, const Byte point);
Byte* deci_to_binary(Byte binary[12], const deci a);

/* math functions */
deci deci_ldexp (deci a, int32_t e);
deci deci_truncate (deci a, deci b);
deci deci_away (deci a, deci b);
deci deci_floor (deci a, deci b);
deci deci_ceil (deci a, deci b);
deci deci_half_even (deci a, deci b);
deci deci_half_away (deci a, deci b);
deci deci_half_truncate (deci a, deci b);
deci deci_half_ceil (deci a, deci b);
deci deci_half_floor (deci a, deci b);
deci deci_sign (deci a);
