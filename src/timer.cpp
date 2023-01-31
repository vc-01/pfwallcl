/*
 * Copyright (c) 2022-2023 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "timer.h"
#include "inifile.h"

#include <dos.h>
#include <mem.h>
#include <limits.h>
#include <assert.h>

struct Timer::internal_state_t
    Timer::internal_state =
        { FALSE, FALSE, -1, 0, 0 };

Timer::time_digits_t::time_digits_t()
{
    memset(digit_arr, -1, sizeof digit_arr);
}

Timer::Timer(
    PFBios::clockspeed_t & const clockspeed) :
    clockspeed (clockspeed),
    int1c_handler_orig_fp(NULL),
    int4a_handler_orig_fp(NULL)
{
}

void Timer::register_handler_int1c(void)
{
#ifndef NTVDM
    int1c_handler_orig_fp = getvect (0x1c);
    setvect (0x1c, & Timer::int1c_handler);
#endif
}

void Timer::register_handler_int4a(void)
{
    int4a_handler_orig_fp = getvect (0x4a);
    setvect (0x4a, & Timer::int4a_handler);
}

void Timer::deregister_handler_int1c(void)
{
    if (int1c_handler_orig_fp)
        setvect (0x1c, int1c_handler_orig_fp);
}

void Timer::deregister_handler_int4a(void)
{
    if (int4a_handler_orig_fp)
        setvect (0x4a, int4a_handler_orig_fp);
}

void Timer::register_handlers(void)
{
    register_handler_int1c();
    register_handler_int4a();
}

void Timer::deregister_handlers(void)
{
    deregister_handler_int4a();
    deregister_handler_int1c();
}

void Timer::set_poweroff_delay_override(
    Timer::DaytimeHHMM const & const poweroff_delay_override_dayt)
{
    asm { cli };
    internal_state.poweroff_delay_override = poweroff_delay_override_dayt.get_abs_min();
    asm { sti };
}

void Timer::unset_poweroff_delay_override()
{
    asm { cli };
    internal_state.poweroff_delay_override = 0;
    asm { sti };
}

void Timer::schedule_next_poweroff(
    INIFile const * const inifile)
{
    /*
        What's behind the monstrous conditional below?
        - User can specify power on at / power off at / power off delay on-keyboard-hit time(s).
        - If either power off or power on time is crossed by khit + power off delay,
          we're leaving from ... or landing in between power on -- power off times;
          -> -power off at- time will be applied as defined in the .ini file,
          kbhit power off delay won't be applied, it will apply otherwise.
        - Either of the above-mentioned config options can be omitted by the user.
    */

    /*
        Legend (apply below):
        'pon'  : power on time as specified in the ini file.
        'poff' : power off time as specified in the ini file.
        'kbhit_poff_delay' : power off delay on keyboard hit as spec. in the ini file.
    */

    /*
        From the priority point of view: pon = poff > kbhit_poff_delay
    */

    asm { cli };
    if (internal_state.poweroff_delay_override)
    {
        set_poweroff_delay_minutes(internal_state.poweroff_delay_override);
        asm { sti };
        return;
    }
    asm { sti };

    struct time
        timep;

    gettime(&timep);

    Timer::DaytimeHHMM const
        now (
            timep.ti_hour, timep.ti_min);
    Timer::DaytimeHHMM const & const
        pon =
            * inifile->pon_dayt_p;
    Timer::DaytimeHHMM const & const
        poff =
            * inifile->poff_dayt_p;
    int
        now_poff_delay =
            inifile->poff_dayt_p ?
                poff - now :
                -1;
    int
        now_pon_delay =
            inifile->pon_dayt_p ?
                pon - now :
                -1;
    unsigned int
        kbhit_poff_delay =
            inifile->kbhit_poff_delay_dayt_p ?
                inifile->kbhit_poff_delay_dayt_p->get_abs_min() :
                DEFAULT_POFF_DELAY_ONKBHIT_MINUTES;
    unsigned int
        now_poff_delay_on_kbhit =
            now_poff_delay < MIN_POFF_DELAY_ONKBHIT_MINUTES ?
                MIN_POFF_DELAY_ONKBHIT_MINUTES :
                now_poff_delay;
    unsigned int
        kbhit_poff_delay_has_crossed_pon =
            now_pon_delay >= 0 ?
                kbhit_poff_delay > now_pon_delay :
                FALSE;
    unsigned int
        kbhit_poff_delay_has_crossed_poff =
            now_poff_delay > 0 ?
                kbhit_poff_delay > now_poff_delay :
                FALSE;
    unsigned int
        in_onperiod_daymode =
            inifile->pon_dayt_p && inifile->poff_dayt_p ?
                pon < poff && (now >= pon && now < poff) :
                FALSE;
    unsigned int
        in_onperiod_nightmode =
            inifile->pon_dayt_p && inifile->poff_dayt_p ?
                pon > poff && (now >= pon || now < poff) :
                FALSE;

    if (kbhit_poff_delay_has_crossed_poff
        || in_onperiod_daymode
        || in_onperiod_nightmode
        ) /* in- or leaving onperiod */
        set_poweroff_delay_minutes(now_poff_delay_on_kbhit);
    else if (kbhit_poff_delay_has_crossed_pon
        ) /* entering on period */
        inifile->poff_dayt_p ?
            set_poweroff_delay_minutes(now_poff_delay_on_kbhit) :
            set_poweroff_delay_minutes(now_pon_delay + kbhit_poff_delay);
    else /* in offperiod */
        set_poweroff_delay_minutes(kbhit_poff_delay);
}

void Timer::set_poweroff_delay_minutes(unsigned int minutes)
{
    if (clockspeed == PFBios::clockspeed_normal)
        set_poweroff_ticks(minutes / 2u);

    else if (clockspeed == PFBios::clockspeed_fast)
        set_poweroff_ticks(minutes * 60l);
}

void Timer::set_poweroff_ticks(unsigned long poweroff_ticks)
{
    poweroff_ticks = MAX(poweroff_ticks, 1);
#ifdef NTVDM
    cout
        << "Timer: Will power-off in "
        << poweroff_ticks
        << " ticks.\n";
#endif
    asm { cli; };
    internal_state.poweroff_now = FALSE;
    internal_state.poweroff_ticks = poweroff_ticks;
    asm { sti; };
}

unsigned int Timer::receive_passed1minute_event(void)
{
    asm { cli };
    unsigned int passed_1minute = internal_state.passed_1minute;
    internal_state.passed_1minute = FALSE;
    asm { sti };
    return passed_1minute;
}

unsigned int Timer::receive_poweroff_event(void)
{
    asm { cli };
    unsigned int poweroff_now = internal_state.poweroff_now;
    internal_state.poweroff_now = FALSE;
    asm { sti };
    return poweroff_now;
}

void
    Timer::reset_events(void)
{
    asm { cli };
    internal_state.passed_1minute = FALSE;
    internal_state.poweroff_now = FALSE;
    internal_state.last_minute_tens = -1;
    internal_state.poweroff_delay_override = 0;
    internal_state.poweroff_ticks = 0;
    asm { sti };
}

struct Timer::timer_events_t Timer::eval_events(
    struct time_digits_t const & const time_digits)
{
    struct Timer::timer_events_t timer_events =
        { FALSE, FALSE, FALSE };

    asm { cli };
    if (internal_state.last_minute_tens == -1)
    {
        goto no_last_minute_tens;
    }
    if (internal_state.last_minute_tens != time_digits.digit.minute_tens)
    {
#ifdef NTVDM
        cout
            << "Timer: passed_10minutes: " << time_digits << "\n";
#endif
        timer_events.passed_10minutes = TRUE;
    }
    if (internal_state.last_minute_tens < 3) // first half of an hour
    {
        if (time_digits.digit.minute_tens >= 3)
        {
#ifdef NTVDM
            cout
                << "Timer: passed_halfanhour: " << time_digits << "\n";
#endif
            timer_events.passed_halfanhour = TRUE;
        }
    }
    else // second half of an hour
    {
        if (time_digits.digit.minute_tens < 3)
        {
#ifdef NTVDM
            cout
                << "Timer: passed_halfanhour: " << time_digits << "\n"
                << "Timer: passed_fullhour: " << time_digits << "\n";
#endif
            timer_events.passed_halfanhour = TRUE;
            timer_events.passed_fullhour = TRUE;
        }
    }

no_last_minute_tens:
    internal_state.last_minute_tens =
        time_digits.digit.minute_tens;
    asm { sti };

    return timer_events;
}

void interrupt Timer::int1c_handler(__CPPARGS)  // Timer tick int. handler;
                                                // called by HW int. handler ->
                                                // end of int. not yet signalled back to
                                                // int. controller, hence it cannot
                                                // be interrupted by another interrupt
{
    if (internal_state.poweroff_ticks > 0)
    {
        if (--internal_state.poweroff_ticks == 0)
        {
            internal_state.last_minute_tens = -1;
            internal_state.poweroff_delay_override = 0;
            internal_state.poweroff_now = TRUE;
        }
    }
}

void interrupt Timer::int4a_handler(__CPPARGS)  // RTC alarm int. handler;
                                                // on wake-up & passed_1minute event
{
    internal_state.passed_1minute = TRUE;
}

#ifdef NTVDM
ostream & const
    operator << (ostream & const out,
        Timer::time_digits_t const & const time_digits)
{
    out <<
        (unsigned int)time_digits.digit.hour_tens <<
        (unsigned int)time_digits.digit.hour_ones <<
        ":" <<
        (unsigned int)time_digits.digit.minute_tens <<
        (unsigned int)time_digits.digit.minute_ones;

    return out;
}
#endif
