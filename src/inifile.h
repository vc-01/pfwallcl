/*
 * Copyright (c) 2022-2023 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _INIFILE_H
#define _INIFILE_H 1

#include "timer.h"

class INIFile
{
    static char const * const
        ini_filename;

public:
    Timer::DaytimeHHMM const // using pointers for nullability
        * const pon_dayt_p,
        * const poff_dayt_p,
        * const kbhit_poff_delay_dayt_p;

    INIFile(
        Timer::DaytimeHHMM const * const,
        Timer::DaytimeHHMM const * const,
        Timer::DaytimeHHMM const * const);

    ~INIFile();

    static INIFile *
        parse();
};

#endif
