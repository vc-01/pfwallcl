/*
 * Copyright (c) 2022-2023 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _PFBIOS_H
#define _PFBIOS_H 1

#define VIDMODE_MDATEXT80x25 0x07
#define VIDMODE_CGA640x200BW 0x06

#define CURSOR_MODE_OFF 0
#define CURSOR_MODE_UNDERLINE 1
#define CURSOR_MODE_BLOCK 2

#include "common.h"

class PFBios
{
public:
    enum clockspeed_t {
        clockspeed_normal = 0, // Tick every 128 seconds (BIOS internal '0')
        clockspeed_fast   = 1, // Tick every second (BIOS internal '1')
        };

    static char const * const
        msg_clockspeed_fast;
    static char const * const
        msg_clockspeed_normal;
    static char const * const
        msg_poweroff_delay_override_deact;
    static char const * const
        msg_poweroff_delay_1h;
    static char * const
        msg_poweroff_delay_h;
    static char const * const
        msg_err_bios_ver;
    static char const * const
        msg_err_wrong_machine;

private:
    inline unsigned char
        dec2bcd(unsigned int);

public:
    int
        check_machinetype(void);
    int
        check_biosver(void);
    void
        init_int61h(void);
    void
        set_videomode(byte_t);
    void
        set_cursor_mode(byte_t);
    void
        read_rtc_time(
            byte_t & const,
            byte_t & const,
            byte_t & const,
            byte_t & const);
    void
        reset_rtc_alarm(void);
    void
        set_rtc_alarm(
            unsigned int,
            unsigned int);
    void
        beep_rndtone(void);
    void
        set_clockspeed(PFBios::clockspeed_t);
    clockspeed_t
        get_clockspeed(void);
    void
        show_message_box(char const *);
    void
        show_message_box_poweroff_delay_h(unsigned int);
    static void
        show_message_earlystage(char const * const);
    void
        poweroff(void);
};

#endif
