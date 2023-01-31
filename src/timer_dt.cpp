/*
 * Copyright (c) 2022 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "timer.h"

#include <math.h>

Timer::DaytimeHHMM::DaytimeHHMM(
    unsigned int hour, unsigned int min)
{
    set(hour, min);
}

unsigned int
    Timer::DaytimeHHMM::set(
        Timer::DaytimeHHMM const & const dayt)
{
    return set(dayt.get_hour(), dayt.get_min());
}

unsigned int
    Timer::DaytimeHHMM::set(
        unsigned int hour, unsigned int min)
{
    unsigned int abs_min = hour * 60 + min;

    while (abs_min > 1440) abs_min -= 1440; // decr. by days

    this->hour = abs_min / 60;
    this->min = abs_min % 60;
    this->abs_min = abs_min;
    return abs_min;
}

unsigned int
    Timer::DaytimeHHMM::get_hour() const
{
    return hour;
}

unsigned int
    Timer::DaytimeHHMM::get_min() const
{
    return min;
}

unsigned int
    Timer::DaytimeHHMM::get_abs_min() const
{
    return abs_min;
}

unsigned int
    Timer::DaytimeHHMM::operator < (
        Timer::DaytimeHHMM const & const dayt) const
{
    return abs_min < dayt.abs_min;
}

unsigned int
    Timer::DaytimeHHMM::operator > (
        Timer::DaytimeHHMM const & const dayt) const
{
    return abs_min > dayt.abs_min;
}

unsigned int
    Timer::DaytimeHHMM::operator >= (
        Timer::DaytimeHHMM const & const dayt) const
{
    return abs_min >= dayt.abs_min;
}

unsigned int
    Timer::DaytimeHHMM::operator + (
        Timer::DaytimeHHMM const & const dayt) const
{
    unsigned int abs_min =
        this->abs_min + dayt.abs_min;

    return abs_min;
}

unsigned int
    Timer::DaytimeHHMM::operator - (
        Timer::DaytimeHHMM const & const dayt) const
{
    signed int abs_min =
        this->abs_min - dayt.abs_min;

    while (abs_min < 0) abs_min += 1440; // incr. by days

    return abs_min;
}

#ifdef NTVDM
ostream & const
    operator << (ostream & const out,
        Timer::DaytimeHHMM const & const dayt)
{
    out.fill('0');
    out.width(2);
    out
        << (unsigned int)dayt.hour
        << ":";
    out.width(2);
    out
        << (unsigned int)dayt.min;

    return out;
}
#endif
