/*
 * Copyright (c) 2022-2023 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifdef TESTS

#include "timer.h"
#include "inifile.h"

#include <time.h>
#include <stdlib.h>
#include <conio.h>
#include <limits.h>
#include <assert.h>

unsigned int Timer::test_poweroff_delay(
    struct dostime_t * const fake_now,
    INIFile const * const inifile,
    unsigned int poweroff_min)
{
    if (poweroff_min < MIN_POFF_DELAY_ONKBHIT_MINUTES)
        poweroff_min = MIN_POFF_DELAY_ONKBHIT_MINUTES;

    set_poweroff_delay_minutes(poweroff_min);
    unsigned long expected_ticks = internal_state.poweroff_ticks;

    _dos_settime (fake_now);
    schedule_next_poweroff(inifile);
    unsigned int real_ticks = internal_state.poweroff_ticks;

    cout.width(5);
    cout
        << "test_poweroff_delay: poweroff_ticks: expected "
        << expected_ticks
        << ", got "
        << real_ticks ;

    if (expected_ticks == real_ticks)
    {
        cout << " -> OK.\n";
        return TRUE;
    }
    else
    {
        cout << " -> FAIL.\n";
        return FALSE;
    }
}

void Timer::test_schedule_next_poweroff(void)
{
    struct dostime_t fake_now = { 0 };
    fake_now.hsecond = 20;  // needs to be here, otherwise the time will be set
                            // a little under the specified full hour.
    putenv("TZ=UTC0");
    tzset();

    INIFile * inifile;

    deregister_handlers();

    inifile = new INIFile(
        NULL,
        NULL,
        NULL);

    // default kbhit_delay
    fake_now.hour = 7;
    fake_now.minute = 0;
    assert(test_poweroff_delay(&fake_now, inifile, DEFAULT_POFF_DELAY_ONKBHIT_MINUTES));

    delete inifile;

    inifile = new INIFile(
        new Timer::DaytimeHHMM(6),
        NULL,
        NULL);

    // default kbhit_delay not crossing pon, before pon
    fake_now.hour = 5;
    fake_now.minute = 0;
    assert(test_poweroff_delay(&fake_now, inifile, DEFAULT_POFF_DELAY_ONKBHIT_MINUTES));

    // default kbhit_delay not crossing pon, after pon
    fake_now.hour = 21;
    fake_now.minute = 0;
    assert(test_poweroff_delay(&fake_now, inifile, DEFAULT_POFF_DELAY_ONKBHIT_MINUTES));

    // default kbhit_delay crossing pon
    fake_now.hour = 5;
    fake_now.minute = 56;
    assert(test_poweroff_delay(&fake_now, inifile, DEFAULT_POFF_DELAY_ONKBHIT_MINUTES + 4));

    delete inifile;

    inifile = new INIFile(
        NULL,
        new Timer::DaytimeHHMM(21),
        NULL);

    // default kbhit_delay not crossing poff
    fake_now.hour = 4;
    fake_now.minute = 0;
    assert(test_poweroff_delay(&fake_now, inifile, DEFAULT_POFF_DELAY_ONKBHIT_MINUTES));

    // default kbhit_delay on poff
    fake_now.hour = 21;
    fake_now.minute = 0;
    assert(test_poweroff_delay(&fake_now, inifile, DEFAULT_POFF_DELAY_ONKBHIT_MINUTES));

    // default kbhit_delay crossing poff
    fake_now.hour = 20;
    fake_now.minute = 58;
    assert(test_poweroff_delay(&fake_now, inifile, MIN_POFF_DELAY_ONKBHIT_MINUTES));

    delete inifile;

    inifile = new INIFile(
        NULL,
        NULL,
        new Timer::DaytimeHHMM(0, 30));

    // kbhit_delay
    fake_now.hour = 4;
    fake_now.minute = 0;
    assert(test_poweroff_delay(&fake_now, inifile, 30));

    // kbhit_delay
    fake_now.hour = 20;
    fake_now.minute = 58;
    assert(test_poweroff_delay(&fake_now, inifile, 30));

    delete inifile;

    inifile = new INIFile(
        NULL,
        new Timer::DaytimeHHMM(21),
        new Timer::DaytimeHHMM(0, 30));

    // kbhit_delay not crossing poff
    fake_now.hour = 4;
    fake_now.minute = 0;
    assert(test_poweroff_delay(&fake_now, inifile, 30));

    // kbhit_delay on poff
    fake_now.hour = 21;
    fake_now.minute = 0;
    assert(test_poweroff_delay(&fake_now, inifile, 30));

    // kbhit_delay crossing poff
    fake_now.hour = 20;
    fake_now.minute = 58;
    assert(test_poweroff_delay(&fake_now, inifile, MIN_POFF_DELAY_ONKBHIT_MINUTES));

    delete inifile;

    inifile = new INIFile(
        new Timer::DaytimeHHMM(4),
        new Timer::DaytimeHHMM(5),
        NULL);

    // default kbhit_delay not crossing poff
    fake_now.hour = 4;
    fake_now.minute = 10;
    assert(test_poweroff_delay(&fake_now, inifile, 50));

    // kbhit_delay crossing pon
    fake_now.hour = 3;
    fake_now.minute = 58;
    assert(test_poweroff_delay(&fake_now, inifile, 2 + 60));

    // kbhit_delay crossing pon
    fake_now.hour = 4;
    fake_now.minute = 0;
    assert(test_poweroff_delay(&fake_now, inifile, 0 + 60));

    // kbhit_delay on poff
    fake_now.hour = 5;
    fake_now.minute = 0;
    assert(test_poweroff_delay(&fake_now, inifile, DEFAULT_POFF_DELAY_ONKBHIT_MINUTES));

    // kbhit_delay crossing poff
    fake_now.hour = 4;
    fake_now.minute = 45;
    assert(test_poweroff_delay(&fake_now, inifile, 15));

    // kbhit_delay
    fake_now.hour = 5;
    fake_now.minute = 1;
    assert(test_poweroff_delay(&fake_now, inifile, DEFAULT_POFF_DELAY_ONKBHIT_MINUTES));

    delete inifile;

    // pon < poff

    inifile = new INIFile(
        new Timer::DaytimeHHMM(6),
        new Timer::DaytimeHHMM(20),
        new Timer::DaytimeHHMM(1, 30));

    fake_now.minute = 0;

    // kbhit between pon .. poff, kbhit_delay not crossing poff
    fake_now.hour = 7;
    assert(test_poweroff_delay(&fake_now, inifile, (20 - 7) * 60));

    // kbhit between pon .. poff, kbhit_delay crossing poff
    fake_now.hour = 19;
    assert(test_poweroff_delay(&fake_now, inifile, (20 - 19) * 60));

    // kbhit between poff .. pon, kbhit_delay not crossing midnight
    fake_now.hour = 22;
    assert(test_poweroff_delay(&fake_now, inifile, 1 * 60 + 30));

    // kbhit between poff .. pon, kbhit_delay crossing midnight
    fake_now.hour = 23;
    assert(test_poweroff_delay(&fake_now, inifile, 1 * 60 + 30));

    // kbhit between poff .. pon, kbhit_delay not crossing pon
    fake_now.hour = 2;
    assert(test_poweroff_delay(&fake_now, inifile, 1 * 60 + 30));

    // kbhit between poff .. pon, kbhit_delay crossing pon
    fake_now.hour = 5;
    assert(test_poweroff_delay(&fake_now, inifile, 1 * 60 + (20 - 6) * 60));

    delete inifile;

    // pon > poff

    inifile = new INIFile(
        new Timer::DaytimeHHMM(20),
        new Timer::DaytimeHHMM(6),
        new Timer::DaytimeHHMM(1, 30));

    // kbhit between pon .. poff, kbhit_delay not crossing midnight
    fake_now.hour = 22;
    assert(test_poweroff_delay(&fake_now, inifile, (24 - 22 + 6) * 60));

    // kbhit between pon .. poff, kbhit_delay crossing midnight
    fake_now.hour = 23;
    assert(test_poweroff_delay(&fake_now, inifile, (24 - 23 + 6) * 60));

    // kbhit between pon .. poff, kbhit_delay not crosing poff
    fake_now.hour = 2;
    assert(test_poweroff_delay(&fake_now, inifile, (6 - 2) * 60));

    // kbhit between pon .. poff, kbhit_delay crosing poff
    fake_now.hour = 5;
    assert(test_poweroff_delay(&fake_now, inifile, (6 - 5) * 60));

    // kbhit between poff .. pon, kbhit_delay not crossing pon
    fake_now.hour = 7;
    assert(test_poweroff_delay(&fake_now, inifile, 1 * 60 + 30));

    // kbhit between poff .. pon, kbhit_delay crossing pon
    fake_now.hour = 19;
    assert(test_poweroff_delay(&fake_now, inifile,  (24 - 19 + 6) * 60));

    delete inifile;

    inifile = new INIFile(
        NULL,
        NULL,
        new Timer::DaytimeHHMM(18));

    // kbhit_delay > UINT_MAX
    fake_now.hour = 0;
    fake_now.minute = 0;
    assert(test_poweroff_delay(&fake_now, inifile, 18 * 60));

    delete inifile;
}

#endif
