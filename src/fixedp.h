/*
 * Copyright (c) 2022-2023 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * Fixed point arithmetic
 */

#ifndef _FIXEDP_H
#define _FIXEDP_H 1

#include "common.h"

#define SCALE 22

// PI: 3.14159265358979311599
#if (SCALE == 20)
#define FIXEDP_PI_RAW 0x3243F6
#elif (SCALE == 22)
#define FIXEDP_PI_RAW 0xC90FDA
#elif (SCALE == 24)
#define FIXEDP_PI_RAW 0x3243F6A
#else
#error "fixedp.h: PI undefined for the SCALE."
#endif

struct Fixedp
{
    typedef signed long fixedp_t; // we have 32 bits for fixed point numbers

    fixedp_t rawvalue;

    Fixedp() :
        rawvalue(0)
    {
    }

    Fixedp(fixedp_t rawvalue, unsigned int raw) :
        rawvalue(rawvalue)
    {
    }

    Fixedp(fixedp_t value) :
        rawvalue(value << SCALE)
    {
    }

#ifdef EMUFPU
    Fixedp(double value) :
        rawvalue((value) * (1l << SCALE))
    {
    }

    double
        to_double() const
    {
        return (double)rawvalue / (1l << SCALE);
    }
#endif

    signed int
        to_integer() const
    {
        return rawvalue / (1l << SCALE);
    }
    
    Fixedp
        operator + (Fixedp const) const;

    friend Fixedp
        operator + (fixedp_t const, Fixedp const);

    void
        operator += (Fixedp const);

    friend void
        operator += (Fixedp & const, fixedp_t const);

    Fixedp
        operator - (void) const;

    Fixedp
        operator - (Fixedp const) const;

    friend Fixedp
        operator - (fixedp_t const, Fixedp const);

    void
        operator -= (Fixedp const);

    friend void
        operator -= (Fixedp & const, fixedp_t const);

    Fixedp
        operator * (Fixedp const) const;

    friend Fixedp
        operator * (fixedp_t const, Fixedp const);

    friend void
        operator *= (Fixedp & const, fixedp_t const);

    void
        operator *= (Fixedp const);

    Fixedp
        operator / (Fixedp const) const;

    friend Fixedp
        operator / (fixedp_t const, Fixedp const);

    static Fixedp
        quasisin_fixedp(Fixedp const);

private:
    unsigned int
        count_leading_zerobits(void) const;
    unsigned int
        count_trailing_zerobits(void) const;
    static Fixedp
        invfact_table(unsigned int const);
};

#endif
