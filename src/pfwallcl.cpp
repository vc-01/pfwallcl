/* 
 * Copyright (c) 2022 Vladimir Chren
 * All rights reserved.
 * 
 * SPDX-License-Identifier: MIT
 */

#include <time.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>

#ifdef NTVDM
	#include <iostream.h>
#endif
	
#ifdef SSHOT
	#include <io.h>
	#include <stdio.h>
	#include <fcntl.h>
	#include <sys\stat.h>
#endif

#include "pfbios.h"
#include "timer.h"
#include "graph.h"
#include "dgclock.h"

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
	set_clockspeed_normal(
		PFBios & const pfbios,
		Timer & const timer)
{
	timer.set_clockspeed_normal();
	pfbios.set_clockspeed(PFBios::clockspeed_normal);
	pfbios.show_message_box(PFBios::msg_clockspeed_normal);
}

void
	set_clockspeed_fast(
		PFBios & const pfbios,
		Timer & const timer)
{
	timer.set_clockspeed_fast();
	pfbios.set_clockspeed(PFBios::clockspeed_fast);
	pfbios.show_message_box(PFBios::msg_clockspeed_fast);
}

int main(void)
{
	randomize();

	PFBios pfbios;

	if (pfbios.check_compat())
	{
		pfbios.show_message_earlystage(PFBios::msg_err_compat);
		return EXIT_FAILURE;
	}

	pfbios.init_int61h();

	if (pfbios.check_bioscompat())
	{
		pfbios.show_message_earlystage(PFBios::msg_err_bioscompat);
		return EXIT_FAILURE;
	}

	pfbios.set_cursor_mode(CURSOR_MODE_OFF);

	PFBios::clockspeed_t
		clockspeed =
			pfbios.get_clockspeed();

	struct Timer::timer_events_t
		timer_events;

	struct Timer::time_digits_t
		time_digits =
			{ { 0, 0, 0, 0 } } ;

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
		clockspeed,
		time_digits);

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
		TRUE,	// refresh_screen
		Graph::DGCLOCK_LEFT_ANIM_RIGHT,	// window_arrangement
		FALSE,	// animate_prep
		FALSE,	// animate_waitnextminute
		FALSE,	// all_cylinders
		TRUE,	// force_passed1minute_event
		FALSE,	// force_poweroff_event
		TRUE,	// do_dgclock_refresh
		TRUE	// do_vram_refresh
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
	do
	{
		// key events processing
		timer.set_poweroff_delay_minkbhit();

		if (c == 'a') // animate toggle switch
		{
			if (!internal_state.all_cylinders &&
				!internal_state.animate_waitnextminute)
			{
				if (clockspeed == PFBios::clockspeed_fast)
				{
					clockspeed = PFBios::clockspeed_normal;
					set_clockspeed_normal(pfbios, timer);
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
		else if (c >= '1' && c <= '9') // power-off to hours numeric key
		{
			unsigned int poweroff_h = c - '0';
			timer.set_poweroff_delay_minutes(poweroff_h * 60);

			if (poweroff_h == 1)
				pfbios.show_message_box(PFBios::msg_poweroff_delay_1h);
			else
				pfbios.show_message_box_poweroff_delay_h(poweroff_h);
			internal_state.refresh_screen = TRUE;
		}
		else if (c == '0') // power-off to default numeric key
		{
			timer.set_poweroff_delay_kbhit();
			pfbios.show_message_box(PFBios::msg_poweroff_delay_m);
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
				set_clockspeed_fast(pfbios, timer);

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
				set_clockspeed_normal(pfbios, timer);
			}
			internal_state.refresh_screen = TRUE;
		}
#ifdef SSHOT
		else if (c == 's') 	// take screenshot, save the file to disk
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
			timer.set_poweroff_delay_minkbhit();
			internal_state.window_arrangement =
				switch_window_arrangement(
					internal_state.window_arrangement,
					graph,
					dgclock);
			internal_state.refresh_screen = TRUE;
		}

		if (internal_state.refresh_screen &&
			(internal_state.all_cylinders ||
			internal_state.animate_waitnextminute))
		{
			internal_state.all_cylinders = FALSE;
			internal_state.animate_waitnextminute = TRUE;
			internal_state.animate_prep = TRUE;
		}

		// events processing loop
		do
		{
			// timer events processing
			if (timer.get_reset_poweroff_event() ||
				internal_state.force_poweroff_event)
			{
				internal_state.force_poweroff_event = FALSE;
				internal_state.force_passed1minute_event = TRUE;
				timer.set_poweroff_delay_minutes(60);	// by using the program,
														// this made itself here;
														// it is somehow expected to have
														// the power off delay elongated
														// from the default 4 minutes
														// after wake up
				pfbios.poweroff();	// power-off _after_ setting power-off ticks,
									// cause the interrupt handler can be fired earlier
									// then the power-off tick count change and the machine
									// could go off immediately after waking up
									// (in the case of one tick to power-off)
			}
			if (timer.get_reset_passed1minute_event() ||
				internal_state.force_passed1minute_event)
			{
				pfbios.read_rtc_time(
					time_digits.digit.hour_tens,
					time_digits.digit.hour_ones,
					time_digits.digit.minute_tens,
					time_digits.digit.minute_ones);

				timer_events = timer.eval_events(time_digits);

				nextalarm_time_t = \
					(unsigned long)time_digits.digit.hour_tens *   36000ul +
					(unsigned long)time_digits.digit.hour_ones *    3600ul +
					(unsigned long)time_digits.digit.minute_tens *   600ul +
					(unsigned long)time_digits.digit.minute_ones *    60ul +
					60ul +
					2ul;	// safety measure if close to the next minute,
							// RTC alarm would never fire up (well, after 24 hours)
							// if late

				nextalarm_tm = gmtime (& nextalarm_time_t);
				
				// BIOS is not firing missed alarms
				// from previous day,
				// schedule the alarm on the next day
				// if close to the end of the current day
				if (nextalarm_tm->tm_hour == 23 &&
					nextalarm_tm->tm_min >= 55)
				{
					nextalarm_tm->tm_hour = 0;
					nextalarm_tm->tm_min = 0;
				}

				pfbios.set_rtc_alarm(
					nextalarm_tm->tm_hour,
					nextalarm_tm->tm_min,
					0);

				if (internal_state.animate_waitnextminute &&
					!internal_state.force_passed1minute_event)
				{
					internal_state.animate_waitnextminute = FALSE;
					internal_state.animate_prep = TRUE;
					internal_state.all_cylinders = TRUE;
				}
				internal_state.force_passed1minute_event = FALSE;
				internal_state.do_dgclock_refresh = TRUE;
			}
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

			// internal states processing
			if (internal_state.refresh_screen)
			{
				internal_state.refresh_screen = FALSE;
#ifndef NTVDM
				graph.cls_withzigzag();
#else // #ifdef NTVDM
				graph.cls_withpattern(0xAAAA);
#endif
				internal_state.do_dgclock_refresh = TRUE;
				if (internal_state.all_cylinders ||
					internal_state.animate_waitnextminute)
				{
					internal_state.animate_prep = TRUE;
				}
			}
#ifdef NTVDM
			if (timer_events.passed_1minute)
			{
				cout << "passed_1minute: " << time_digits << " " << endl;
			}
			if (timer_events.passed_10minutes)
			{
				cout << "passed_10minutes: " << time_digits << " " << endl;
			}
			if (timer_events.passed_halfanhour)
			{
				cout << "passed_halfanhour: " << time_digits << " " << endl;
			}
			if (timer_events.passed_fullhour)
			{
				cout << "passed_fullhour: " << time_digits << " " << endl;
			}
			if (wherey() >= 25)
				gotoxy(1,18);
#endif
			*(byte_t *)(&timer_events) = 0; // bad idea, but fast:
											//   mov	byte ptr [bp-6],0

											// instead of:
											//   timer_events.passed_1minute = FALSE;
											//   timer_events.passed_10minutes = FALSE;
											//   timer_events.passed_halfanhour = FALSE;
											//   timer_events.passed_fullhour = FALSE;
											//   timer_events.poweroff = FALSE;
											//   and	byte ptr [bp-6],254
											//   and	byte ptr [bp-6],253
											//   and	byte ptr [bp-6],251
											//   and	byte ptr [bp-6],247
											//   and	byte ptr [bp-6],239

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
				asm hlt;	// <- this will halt the PF interruptibly
							// for either 1 second, or 128 seconds
							// depending on the current clock tick speed

#ifdef NTVDM
				delay(50);
#endif
			}
		}
		while (!kbhit());	// <- this will reset the PF's
							// internal power-off ticks counter
	}
	while ((c = getch()) != ESC_CHAR);

exit:
	pfbios.set_clockspeed(PFBios::clockspeed_normal);
	pfbios.set_cursor_mode(CURSOR_MODE_BLOCK);
	pfbios.set_videomode(VIDMODE_MDATEXT80x25);

	return EXIT_SUCCESS;
}
