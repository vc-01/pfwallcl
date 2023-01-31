/*
 * Copyright (c) 2022-2023 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "pfbios.h"
#include "timer.h"
#include "graph.h"
#include "dgclock.h"
#include "inifile.h"

#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <time.h>
#include <string.h>

#ifdef NTVDM
#include <iostream.h>
#endif

#ifdef SSHOT
#include <io.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys\stat.h>
#endif

#define ESC_CHAR '\33'
#define SPACE_CHAR ' '

Graph::window_arrangement_t
    switch_window_arrangement(
        Graph::window_arrangement_t const window_arrangement,
        Graph & const graph,
        DgClock & const dgclock)
{
    Graph::window_arrangement_t new_window_arrangement =
        window_arrangement;

    if (window_arrangement ==
        Graph::DGCLOCK_LEFT_ANIM_RIGHT)
    {
        new_window_arrangement =
            Graph::DGCLOCK_RIGHT_ANIM_LEFT;
        graph.set_window_arrangement(
            new_window_arrangement);
        dgclock.set_window_arrangement(
            new_window_arrangement);
    }
    else if (window_arrangement ==
        Graph::DGCLOCK_RIGHT_ANIM_LEFT)
    {
        new_window_arrangement =
            Graph::DGCLOCK_LEFT_ANIM_RIGHT;
        graph.set_window_arrangement(
            new_window_arrangement);
        dgclock.set_window_arrangement(
            new_window_arrangement);
    }

    return new_window_arrangement;
}

void
    set_clockspeed(
        PFBios & const pfbios,
        PFBios::clockspeed_t const & const clockspeed)
{
    pfbios.set_clockspeed(clockspeed);
    if (clockspeed == PFBios::clockspeed_normal)
        pfbios.show_message_box(PFBios::msg_clockspeed_normal);
    if (clockspeed == PFBios::clockspeed_fast)
        pfbios.show_message_box(PFBios::msg_clockspeed_fast);
}

int main(int const argc, char * const * const argv)
{
    int do_check_biosver = TRUE;

    if (argc > 1)
    {
        int unsupported_arg = FALSE;

        for (char * const * arg = argv + 1 ; * arg != NULL ; arg++)
        {
            if (strcmp(*arg, "untested") == 0)
            {
                do_check_biosver = FALSE;
            }
            else
            {
                char s [40] = "Unknown argument: ";
                strncat(s, *arg, 15);
                strncat(s, "\r\n$", 3);

                PFBios::show_message_earlystage(s);
                unsupported_arg = TRUE;
            }
        }

        if (unsupported_arg)
        {
            PFBios::show_message_earlystage("ERR: Wrong argument(s).\r\n$");
            return EXIT_FAILURE;
        }
    }

    INIFile * inifile = INIFile::parse();

    if (!inifile)
    {
        PFBios::show_message_earlystage("ERR: Ini file error(s).\r\n$");
        return EXIT_FAILURE;
    }
    
    randomize();

    PFBios pfbios;

    if (pfbios.check_machinetype())
    {
        pfbios.show_message_earlystage(PFBios::msg_err_wrong_machine);
        return EXIT_FAILURE;
    }

    pfbios.init_int61h();

    if (do_check_biosver && pfbios.check_biosver())
    {
        pfbios.show_message_earlystage(PFBios::msg_err_bios_ver);
        return EXIT_FAILURE;
    }

    pfbios.set_cursor_mode(CURSOR_MODE_OFF);

    PFBios::clockspeed_t
        clockspeed =
            pfbios.get_clockspeed();

    struct Timer::timer_events_t
        timer_events;

    struct Timer::time_digits_t
        time_digits;

    time_t
        nextalarm_time_t;

    struct tm *
        nextalarm_tm;

    pfbios.read_rtc_time(
        time_digits.digit.hour_tens,
        time_digits.digit.hour_ones,
        time_digits.digit.minute_tens,
        time_digits.digit.minute_ones);

    Timer timer = Timer (
        clockspeed);

#ifdef TESTS
    clockspeed = PFBios::clockspeed_normal;
    timer.test_schedule_next_poweroff();
    clockspeed = PFBios::clockspeed_fast;
    timer.test_schedule_next_poweroff();
    cout << "OK: All tests passed.\n";
    return EXIT_SUCCESS;
#endif

    timer.register_handlers();

    struct internal_state_t {
        unsigned int refresh_screen : 1;
        Graph::window_arrangement_t window_arrangement : 2;
        unsigned int animate_prep : 1;
        unsigned int animate_waitnextminute : 1;
        unsigned int all_cylinders : 1;
        unsigned int force_passed1minute_event : 1;
        unsigned int force_poweroff_event : 1;
        unsigned int do_dgclock_refresh : 1;
        unsigned int do_vram_refresh : 1;
    } internal_state = {
        TRUE,   // refresh_screen
        Graph::DGCLOCK_LEFT_ANIM_RIGHT, // window_arrangement
        FALSE,  // animate_prep
        FALSE,  // animate_waitnextminute
        FALSE,  // all_cylinders
        TRUE,   // force_passed1minute_event
        FALSE,  // force_poweroff_event
        TRUE,   // do_dgclock_refresh
        TRUE    // do_vram_refresh
    };

    Graph graph =
        Graph(
            internal_state.window_arrangement);

    DgClock dgclock =
        DgClock(
            internal_state.window_arrangement);

    pfbios.set_videomode(VIDMODE_CGA640x200BW);

    int c = 0;
#ifdef NTVDM
    gotoxy(1,18);
#endif

    // kbhit loop
    do
    {
        if (c == 'a') // toggle animation
        {
            if (!internal_state.all_cylinders &&
                !internal_state.animate_waitnextminute)
            {
                if (clockspeed == PFBios::clockspeed_fast)
                {
                    clockspeed = PFBios::clockspeed_normal;
                    set_clockspeed(pfbios, clockspeed);
                }
                internal_state.animate_waitnextminute = TRUE;
                pfbios.show_message_box(PFBios::msg_anim_willstart);
            }
            else
            {
                internal_state.animate_prep = FALSE;
                internal_state.animate_waitnextminute = FALSE;
                internal_state.all_cylinders = FALSE;
            }
            internal_state.refresh_screen = TRUE;
        }
        else if (c >= '0' && c <= '9') // override power-off delay
        {
            unsigned int numkey = c - '0';

            if (numkey == 0) // reset power-off delay back as set in the .ini file
                timer.unset_poweroff_delay_override();
            else // set power-off delay to <numkey> hours
                timer.set_poweroff_delay_override(
                    Timer::DaytimeHHMM (numkey));

            if (numkey == 0)
                pfbios.show_message_box(PFBios::msg_poweroff_delay_override_deact);
            else if (numkey == 1)
                pfbios.show_message_box(PFBios::msg_poweroff_delay_1h);
            else
                pfbios.show_message_box_poweroff_delay_h(numkey);

            internal_state.refresh_screen = TRUE;
        }
        else if (c == 'o') // power-off now
        {
            internal_state.force_poweroff_event = TRUE;
        }
        else if (c == 'f') // fast-tick toggle
        {
            if (clockspeed == PFBios::clockspeed_normal)
            {
                clockspeed = PFBios::clockspeed_fast;
                set_clockspeed(pfbios, clockspeed);

                if (internal_state.all_cylinders ||
                    internal_state.animate_waitnextminute)
                {
                    pfbios.show_message_box(PFBios::msg_anim_willstop);
                }
                internal_state.animate_prep = FALSE;
                internal_state.animate_waitnextminute = FALSE;
                internal_state.all_cylinders = FALSE;
            }
            else if (clockspeed == PFBios::clockspeed_fast)
            {
                clockspeed = PFBios::clockspeed_normal;
                set_clockspeed(pfbios, clockspeed);
            }
            internal_state.refresh_screen = TRUE;
        }
#ifdef SSHOT
        else if (c == 's')  // take screenshot, save the file to disk
                            // and exit
        {
            int fd;
            int res;
            char const * const filename = "sshot.bin";

            fd = open(filename,
                O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,
                S_IREAD | S_IWRITE);

            if (fd == -1)
            {
                pfbios.set_videomode(VIDMODE_MDATEXT80x25);
                perror ("Screenshot: Open file");
                goto exit;
            }

            res = graph.take_screenshot(fd);

            if (res == -1)
            {
                pfbios.set_videomode(VIDMODE_MDATEXT80x25);
                perror ("Screenshot: Write");
                goto exit;
            }
            if (res == -2)
            {
                pfbios.set_videomode(VIDMODE_MDATEXT80x25);
                fprintf (stderr, "Screenshot: Couldn't write the buffer to file.");
                goto exit;
            }

            res = close (fd);

            if (res != 0)
            {
                pfbios.set_videomode(VIDMODE_MDATEXT80x25);
                perror ("Screenshot: Close file");
                goto exit;
            }

            goto exit;
        }
#endif
        else if (c == SPACE_CHAR) // arcade
        {
#ifdef NTVDM
            internal_state.force_passed1minute_event = TRUE;
            pfbios.beep_rndtone();
#endif
            internal_state.window_arrangement =
                switch_window_arrangement(
                    internal_state.window_arrangement,
                    graph,
                    dgclock);
            internal_state.refresh_screen = TRUE;
        }

        if (! internal_state.force_poweroff_event)
        {
            timer.schedule_next_poweroff(inifile);
        }
        if (internal_state.refresh_screen &&
            (internal_state.all_cylinders ||
            internal_state.animate_waitnextminute))
        {
            internal_state.all_cylinders = FALSE;
            internal_state.animate_waitnextminute = TRUE;
            internal_state.animate_prep = TRUE;
        }

        // on timer interrupt
        do
        {
            // timer events
            if (timer.receive_poweroff_event() ||
                internal_state.force_poweroff_event)
            {
                internal_state.force_poweroff_event = FALSE;
                pfbios.reset_rtc_alarm();
                if (inifile->pon_dayt_p)
                    pfbios.set_rtc_alarm(
                        inifile->pon_dayt_p->get_hour(),
                        inifile->pon_dayt_p->get_min());
                pfbios.poweroff();
                // zzz...
#ifndef NTVDM
                pfbios.beep_rndtone();
                pfbios.beep_rndtone();
                pfbios.beep_rndtone();
#endif
                timer.reset_events();
                timer.schedule_next_poweroff(inifile);
                internal_state.force_passed1minute_event = TRUE;
            }
            if (timer.receive_passed1minute_event() ||
                internal_state.force_passed1minute_event)
            {
                pfbios.read_rtc_time(
                    time_digits.digit.hour_tens,
                    time_digits.digit.hour_ones,
                    time_digits.digit.minute_tens,
                    time_digits.digit.minute_ones);

                timer_events = timer.eval_events(time_digits);

                nextalarm_time_t =
                    (unsigned long)time_digits.digit.hour_tens *   36000ul +
                    (unsigned long)time_digits.digit.hour_ones *    3600ul +
                    (unsigned long)time_digits.digit.minute_tens *   600ul +
                    (unsigned long)time_digits.digit.minute_ones *    60ul +
                    60ul +
                    2ul;    // safety measure if close to next minute,
                            // if late, RTC alarm would never fire (well, after 24 hours)

                nextalarm_tm = gmtime (& nextalarm_time_t);

                // BIOS is not firing missed alarms from the previous day,
                // if close to the end of the day,
                // schedule the alarm rather on the next day
                if (nextalarm_tm->tm_hour == 23 &&
                    nextalarm_tm->tm_min >= 55)
                {
                    nextalarm_tm->tm_hour = 0;
                    nextalarm_tm->tm_min = 0;
                }

                pfbios.reset_rtc_alarm();

                pfbios.set_rtc_alarm(
                    nextalarm_tm->tm_hour,
                    nextalarm_tm->tm_min);

                if (internal_state.animate_waitnextminute &&
                    !internal_state.force_passed1minute_event)
                {
                    internal_state.animate_waitnextminute = FALSE;
                    internal_state.animate_prep = TRUE;
                    internal_state.all_cylinders = TRUE;
                }
                internal_state.force_passed1minute_event = FALSE;
                internal_state.do_dgclock_refresh = TRUE;

                if (timer_events.passed_10minutes)
                {
                    internal_state.window_arrangement =
                        switch_window_arrangement(
                            internal_state.window_arrangement,
                            graph,
                            dgclock);
                    internal_state.refresh_screen = TRUE;
                }
                if (timer_events.passed_halfanhour)
                {
                    pfbios.beep_rndtone();
                }
                if (timer_events.passed_fullhour)
                {
                    pfbios.beep_rndtone();
                }
            }

            // internal state events
            if (internal_state.refresh_screen)
            {
                internal_state.refresh_screen = FALSE;
#ifdef NTVDM
                graph.cls_withpattern(0);
#else
                graph.cls_withzigzag();
#endif
                internal_state.do_dgclock_refresh = TRUE;
                if (internal_state.all_cylinders ||
                    internal_state.animate_waitnextminute)
                {
                    internal_state.animate_prep = TRUE;
                }
            }

            if (internal_state.do_dgclock_refresh)
            {
                internal_state.do_dgclock_refresh = FALSE;
                internal_state.do_vram_refresh = TRUE;
                dgclock.draw(time_digits);
            }
            if (internal_state.animate_prep)
            {
                internal_state.animate_prep = FALSE;
                internal_state.do_vram_refresh = TRUE;
                graph.anim_prep();
            }
            if (internal_state.all_cylinders)
            {
                internal_state.do_vram_refresh = TRUE;
                if (graph.animate_finished())
                {
                    internal_state.all_cylinders = FALSE;
                    internal_state.animate_waitnextminute = TRUE;
                }
            }
            if (internal_state.do_vram_refresh)
            {
                internal_state.do_vram_refresh = FALSE;
                graph.vram_copy();
            }
            if (!internal_state.all_cylinders)
            {
                asm hlt;    // <- this will halt POFO interruptibly,
                            // for either 1 second, or ~2 minutes;
                            // depending on the current clocktick speed

#ifdef NTVDM
                delay(50);
#endif
            }
        }
        while (!kbhit());   // <- this will reset the POFO's
                            // internal power-off ticks counter
    }
    while ((c = getch()) != ESC_CHAR);

exit:
    timer.deregister_handlers();
    pfbios.set_clockspeed(PFBios::clockspeed_normal);
    pfbios.set_cursor_mode(CURSOR_MODE_BLOCK);
    pfbios.set_videomode(VIDMODE_MDATEXT80x25);

    return EXIT_SUCCESS;
}
