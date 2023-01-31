/*
 * Copyright (c) 2022-2023 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "fixedp.h"

Fixedp
    Fixedp::operator + (Fixedp const addend) const
{
    return Fixedp(rawvalue + addend.rawvalue, TRUE);
}

Fixedp
    operator + (Fixedp::fixedp_t const addend_1, Fixedp const addend_2)
{
    return Fixedp(addend_1) + addend_2;
}

void
    Fixedp::operator += (Fixedp const addend)
{
    rawvalue += addend.rawvalue;
}

void
    operator += (Fixedp & const addend_1, Fixedp::fixedp_t const addend_2)
{
    addend_1 += Fixedp(addend_2);
}

Fixedp
    Fixedp::operator - (void) const
{
    return Fixedp(-rawvalue, TRUE);
}

Fixedp
    Fixedp::operator - (Fixedp const subtrahend) const
{
    return Fixedp(rawvalue - subtrahend.rawvalue, TRUE);
}

Fixedp
    operator - (Fixedp::fixedp_t const minuend, Fixedp const subtrahend)
{
    return Fixedp(minuend) - subtrahend;
}

void
    Fixedp::operator -= (Fixedp const subtrahend)
{
    rawvalue -= subtrahend.rawvalue;
}

void
    operator -= (Fixedp & const minuend, Fixedp::fixedp_t const subtrahend)
{
    minuend -= Fixedp(subtrahend);
}

// fixed point multiplicate without data type bit width expansion
Fixedp
    Fixedp::operator * (Fixedp const multiplicant) const
{
    Fixedp::fixedp_t
        x = rawvalue,
        y = multiplicant.rawvalue;

    unsigned int clz_x = 0;
    unsigned int clz_y = 0;
    if (x < 0)
        clz_x = Fixedp(- *this).count_leading_zerobits();
    else
        clz_x = count_leading_zerobits();
    if (y < 0)
        clz_y = Fixedp(-multiplicant).count_leading_zerobits();
    else
        clz_y = multiplicant.count_leading_zerobits();
    unsigned int ctz_x = count_trailing_zerobits();
    unsigned int ctz_y = multiplicant.count_trailing_zerobits();
    unsigned int scale_rshift_x = ctz_x;
    unsigned int scale_rshift_y = ctz_y;
    while ((clz_x + scale_rshift_x) + (clz_y + scale_rshift_y) <= BITS_PER_LONG - 2 /* 2-times of sign bit */)
    {
        if (x > y) scale_rshift_x++; else scale_rshift_y++;
    }

    int scale_rshift = scale_rshift_x + scale_rshift_y;
    Fixedp::fixedp_t res_raw = (x >> scale_rshift_x) * (y >> scale_rshift_y);

    if (scale_rshift > SCALE)
        res_raw <<= scale_rshift - SCALE;
    else
        res_raw >>= SCALE - scale_rshift;

    return Fixedp(res_raw, TRUE);
}

Fixedp
    operator * (Fixedp::fixedp_t const multiplicant_1, Fixedp const multiplicant_2)
{
    return Fixedp(multiplicant_1) * multiplicant_2;
}

void
    Fixedp::operator *= (Fixedp const multiplicant)
{
    rawvalue = (* this * multiplicant).rawvalue;
}

void
    operator *= (Fixedp & const multiplicant_1, Fixedp::fixedp_t const multiplicant_2)
{
    multiplicant_1 *= Fixedp(multiplicant_2);
}

// fixed point division without data type bit width expansion
Fixedp
    Fixedp::operator / (Fixedp const divisor) const
{
    Fixedp::fixedp_t
        x = rawvalue,
        y = divisor.rawvalue;

    unsigned int clz_x = 0;
    unsigned int clz_y = 0;
    if (x < 0)
        clz_x = Fixedp(- *this).count_leading_zerobits();
    else
        clz_x = count_leading_zerobits();
    if (y < 0)
        clz_y = Fixedp(-divisor).count_leading_zerobits();
    else
        clz_y = divisor.count_leading_zerobits();
    unsigned int ctz_y = divisor.count_trailing_zerobits();
    unsigned int scale_lshift_x = clz_x;
    unsigned int scale_rshift_y = ctz_y;
    while (clz_x + (clz_y + scale_rshift_y) <= BITS_PER_LONG/2 - 2 /* 2-times of sign bit */)
    {
        scale_rshift_y++;
    }
    int scale_shift = scale_lshift_x + scale_rshift_y;
    Fixedp::fixedp_t res_raw = (x << scale_lshift_x) / (y >> scale_rshift_y);

    if (scale_shift > SCALE)
        res_raw >>= scale_shift - SCALE;
    else
        res_raw <<= SCALE - scale_shift;

    return Fixedp(res_raw, TRUE);
}

Fixedp
    operator / (Fixedp::fixedp_t const divident, Fixedp const divisor)
{
    return Fixedp(divident) / divisor;
}

unsigned int
    Fixedp::count_trailing_zerobits(void) const
{
/*
 * © 1997-2005 Sean Eron Anderson.
 *
 * The code and descriptions are distributed in the hope that they will be
 * useful, but WITHOUT ANY WARRANTY and without even the implied warranty
 * of merchantability or fitness for a particular purpose.
 *
 * Source URL: https://graphics.stanford.edu/~seander/bithacks.html
 */
    unsigned long v = (unsigned long)rawvalue; // 32-bit word input to count zero bits on right
    unsigned int c = 32; // c will be the number of zero bits on the right
    v &= -(signed long)(v);
    if (v) c--;
    if (v & 0x0000FFFF) c -= 16;
    if (v & 0x00FF00FF) c -= 8;
    if (v & 0x0F0F0F0F) c -= 4;
    if (v & 0x33333333) c -= 2;
    if (v & 0x55555555) c -= 1;
    return c;
}

unsigned int
    Fixedp::count_leading_zerobits(void) const
{
/*
 * Algorithm taken from: https://en.wikipedia.org/wiki/Find_first_set#CLZ
 */
    unsigned long v = (unsigned long)rawvalue; // 32-bit word input to count zero bits on left
    if (v == 0) return 32;
    unsigned int c = 0; // c will be the number of zero bits on the left
    if (! (v & 0xFFFF0000)) { c += 16; v <<= 16; }
    if (! (v & 0xFF000000)) { c += 8; v <<= 8; }
    if (! (v & 0xF0000000)) { c += 4; v <<= 4; }
    if (! (v & 0xC0000000)) { c += 2; v <<= 2; }
    if (! (v & 0x80000000)) { c += 1; }
    return c > 0 ? c - 1 : c;
}

Fixedp
    Fixedp::invfact_table(unsigned int const n)
{
    Fixedp::fixedp_t res = 0;
    switch(n)
    {
        case 2: res =  (1l << SCALE) / (1l * 2 * 1); break; // borland c++ 3.1 needs the first constant of "long" type
        case 3: res =  (1l << SCALE) / (1l * 3 * 2 * 1); break;
        case 4: res =  (1l << SCALE) / (1l * 4 * 3 * 2 * 1); break;
        case 5: res =  (1l << SCALE) / (1l * 5 * 4 * 3 * 2 * 1); break;
        case 6: res =  (1l << SCALE) / (1l * 6 * 5 * 4 * 3 * 2 * 1); break;
        case 7: res =  (1l << SCALE) / (1l * 7 * 6 * 5 * 4 * 3 * 2 * 1); break;
        case 8: res =  (1l << SCALE) / (1l * 8 * 7 * 6 * 5 * 4 * 3 * 2 * 1); break;
        case 9: res =  (1l << SCALE) / (1l * 9 * 8 * 7 * 6 * 5 * 4 * 3 * 2 * 1); break;
        case 10: res = (1l << SCALE) / (1l * 10 * 9 * 8 * 7 * 6 * 5 * 4 * 3 * 2 * 1); break;
        case 11: res = (1l << SCALE) / (1l * 11 * 10 * 9 * 8 * 7 * 6 * 5 * 4 * 3 * 2 * 1); break;
        default: 1l << SCALE; break;
    }

    return Fixedp(res, TRUE);
}

#define TERM_MAX 5
Fixedp
    Fixedp::quasisin_fixedp(Fixedp const xrad)
{
    Fixedp::fixedp_t xrad_raw = xrad.rawvalue;
    unsigned int in_second_halfperiod = FALSE;
    while (xrad_raw > 2 * FIXEDP_PI_RAW) xrad_raw -= 2 * FIXEDP_PI_RAW;
    if (xrad_raw > FIXEDP_PI_RAW) { xrad_raw -= FIXEDP_PI_RAW; in_second_halfperiod = TRUE; }
    if (xrad_raw > FIXEDP_PI_RAW / 2) xrad_raw = FIXEDP_PI_RAW - xrad_raw;

    // Maclaurin series polynomial
    Fixedp xrad_norm (xrad_raw, TRUE);
    Fixedp sinx = xrad_norm;
    Fixedp term;
    Fixedp term_powx = xrad_norm;
    register signed int term_sign = -1;

    for (int n = 3; n <= TERM_MAX; n += 2)
    {
        term_powx *= xrad_norm * xrad_norm;
        term = term_powx * invfact_table(n);
        sinx += term_sign > 0 ? term : -term;
        asm { neg WORD PTR term_sign };
    }

    if (in_second_halfperiod) sinx = -sinx;

    return sinx;
}
