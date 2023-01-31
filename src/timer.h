/*
 * Copyright (c) 2022-2023 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _TIMER_H
#define _TIMER_H 1

#include "pfbios.h"
#include "common.h"

#ifdef NTVDM
#include <iostream.h>
#include <dos.h>
#endif

#ifdef __cplusplus
    #define __CPPARGS ...
#else
    #define __CPPARGS
#endif

class INIFile;

class Timer
{
public:
    class DaytimeHHMM
    {
        byte_t hour;
        byte_t min;
        unsigned int abs_min;

    public:
        DaytimeHHMM(unsigned int, unsigned int = 0);

        unsigned int
            set(DaytimeHHMM const & const);
        unsigned int
            set(unsigned int, unsigned int = 0);
        unsigned int
            get_hour() const;
        unsigned int
            get_min() const;
        unsigned int
            get_abs_min() const;

        unsigned int
            operator < (DaytimeHHMM const & const) const;
        unsigned int
            operator > (DaytimeHHMM const & const) const;
        unsigned int
            operator >= (DaytimeHHMM const & const) const;
        unsigned int
            operator + (DaytimeHHMM const & const) const;
        unsigned int
            operator - (DaytimeHHMM const & const) const;
#ifdef NTVDM
        friend ostream & const
            operator << (
                ostream & const, DaytimeHHMM const & const);
#endif
    };

    struct time_digits_t {
        union {
            struct time_digit_t {
                byte_t hour_tens;
                byte_t hour_ones;
                byte_t minute_tens;
                byte_t minute_ones;
            } digit;
            byte_t digit_arr[4];
        };
        time_digits_t();
#ifdef NTVDM
        friend ostream & const
            operator << (ostream & const, time_digits_t const & const);
#endif
    };

    struct timer_events_t {
        unsigned int passed_10minutes : 1;
        unsigned int passed_halfanhour : 1;
        unsigned int passed_fullhour : 1;
    };

private:
    struct internal_state_t
    {
        unsigned int passed_1minute : 1;
        unsigned int poweroff_now : 1;
        char last_minute_tens;
        unsigned int poweroff_delay_override;
        unsigned long poweroff_ticks;
    };

    static struct internal_state_t internal_state;

    PFBios::clockspeed_t const & const
        clockspeed;

    void
        set_poweroff_delay_minutes(unsigned int);
    void
        set_poweroff_ticks(unsigned long);
    void interrupt
        (* int1c_handler_orig_fp)(__CPPARGS);
    void interrupt
        (* int4a_handler_orig_fp)(__CPPARGS);
    static void
        interrupt int1c_handler(__CPPARGS);
    static void
        interrupt int4a_handler(__CPPARGS);
    void
        register_handler_int1c(void);
    void
        register_handler_int4a(void);
    void
        deregister_handler_int1c(void);
    void
        deregister_handler_int4a(void);

public:
    Timer(
        PFBios::clockspeed_t & const);
    void
        register_handlers(void);
    void
        deregister_handlers(void);
    struct Timer::timer_events_t
        eval_events(struct time_digits_t const & const);
    unsigned int
        receive_passed1minute_event(void);
    unsigned int
        receive_poweroff_event(void);
    void
        reset_events(void);
    void
        set_poweroff_delay_override(Timer::DaytimeHHMM const & const);
    void
        unset_poweroff_delay_override();
    void
        schedule_next_poweroff(
            INIFile const * const);
#ifdef TESTS
    unsigned int
        test_poweroff_delay(
            struct dostime_t * const,
            INIFile const * const,
            unsigned int);
    void
        test_schedule_next_poweroff(void);
#endif
};

#endif
