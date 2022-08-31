/* 
 * Copyright (c) 2022 Vladimir Chren
 * All rights reserved.
 * 
 * SPDX-License-Identifier: MIT
 */

#include <dos.h>

#include "timer.h"

#define ONKBHIT_POWEROFF_MINUTES 4
#define CF 1

struct internal_state_t
{
	int minute_tens_last;
	int poweroff_ticks;
} internal_state = { -1, 0 };

struct Timer::timer_events_t timer_events =
	{ FALSE, FALSE, FALSE, FALSE, FALSE };

Timer::Timer(
	PFBios::clockspeed_t & const clockspeed,
	struct time_digits_t const & const time_digits) :
	clockspeed (clockspeed)
{
	set_poweroff_delay_kbhit();	// set default power-off timeout

	eval_events(time_digits);		// force the evaluation now
									// with that, prepare the initial state

	int1c_handler_orig_fp = getvect (0x1c);
#ifndef NTVDM
	setvect (0x1c, & Timer::int1c_handler);
#endif
	int4a_handler_orig_fp = getvect (0x4a);
	setvect (0x4a, & Timer::int4a_handler);
}

Timer::~Timer(void)
{
	setvect (0x1c, int1c_handler_orig_fp);
	setvect (0x4a, int4a_handler_orig_fp);
}

void Timer::set_clockspeed_normal(void)
{
	set_poweroff_ticks(internal_state.poweroff_ticks / 120);
}
void Timer::set_clockspeed_fast(void)
{
	set_poweroff_ticks(internal_state.poweroff_ticks * 120);
}

void Timer::set_poweroff_ticks(unsigned int poweroff_ticks)
{
	struct REGPACK regpack;
	regpack.r_ax = poweroff_ticks;
	regpack.r_bx = FP_OFF (& internal_state.poweroff_ticks);

	asm {
		push ax
		push bx
		mov ax, word ptr regpack.r_ax
		mov bx, word ptr regpack.r_bx
		lock xchg word ptr [ bx ], ax
		pop bx
		pop ax
	}

	timer_events.poweroff = FALSE;
}

void Timer::dec_poweroff_ticks()
{
	struct REGPACK regpack;
	regpack.r_bx = FP_OFF (& internal_state.poweroff_ticks);

	asm {
		push bx
		mov bx, word ptr regpack.r_bx
		lock dec word ptr [ bx ]
		pop bx
	}
}

int Timer::get_poweroff_delay_kbhit_ticks()
{
	if (clockspeed == PFBios::clockspeed_normal)
	{
		return ONKBHIT_POWEROFF_MINUTES / 2; // ONKBHIT_POWEROFF_MINUTES to 2-minute tick
	}
	else if (clockspeed == PFBios::clockspeed_fast)
	{
		return ONKBHIT_POWEROFF_MINUTES * 60; // ONKBHIT_POWEROFF_MINUTES to 1-second tick
	}
	return 0;
}

void Timer::set_poweroff_delay_minutes(unsigned int minutes)
{
	if (clockspeed == PFBios::clockspeed_normal)
	{
		set_poweroff_ticks( MAX(minutes, 2u) / 2 ); // minutes to 2-minute tick
	}
	else if (clockspeed == PFBios::clockspeed_fast)
	{
		set_poweroff_ticks( MAX(minutes, 1u) * 60); // minutes to 1-second tick
	}
}

void Timer::set_poweroff_delay_kbhit(void)
{
	set_poweroff_delay_minutes(ONKBHIT_POWEROFF_MINUTES);
}

void Timer::set_poweroff_delay_minkbhit(void)
{
	if (internal_state.poweroff_ticks < get_poweroff_delay_kbhit_ticks())
	{
		set_poweroff_delay_minutes(ONKBHIT_POWEROFF_MINUTES);
	}
}

unsigned int Timer::get_reset_poweroff_event(void)
{
	struct timer_events_t timer_events_snapshot = timer_events;
	timer_events.poweroff = FALSE;
	return timer_events_snapshot.poweroff;
}

unsigned int Timer::get_reset_passed1minute_event(void)
{
	struct timer_events_t timer_events_snapshot = timer_events;
	timer_events.passed_1minute = FALSE;
	return timer_events_snapshot.passed_1minute;
}

struct Timer::timer_events_t Timer::eval_events(
	struct time_digits_t const & const time_digits)
{
	struct Timer::timer_events_t timer_events =
		{ TRUE, FALSE, FALSE, FALSE, FALSE };

	if (internal_state.minute_tens_last == -1)
	{
		goto no_minute_tens_last;
	}
	if (internal_state.minute_tens_last != time_digits.digit.minute_tens)
	{
		timer_events.passed_10minutes = TRUE;
	}
	if (internal_state.minute_tens_last < 3) // first half of an hour
	{
		if (time_digits.digit.minute_tens >= 3)
		{
			timer_events.passed_halfanhour = TRUE;
		}
	}
	else // second half of an hour
	{
		if (time_digits.digit.minute_tens < 3)
		{
			timer_events.passed_halfanhour = TRUE;
			timer_events.passed_fullhour = TRUE;
		}
	}

no_minute_tens_last:
	internal_state.minute_tens_last =
		time_digits.digit.minute_tens;

	return timer_events; 	// return a copy, this is atomic,
							// no locking required
}

void interrupt Timer::int1c_handler(__CPPARGS)	// timer interrupt;
												// shot every tick
{
	if (internal_state.poweroff_ticks == 0)
		timer_events.poweroff = TRUE;
	else
		dec_poweroff_ticks();
}

void interrupt Timer::int4a_handler(__CPPARGS)	// RTC alarm interrupt;
												// shot every minute
{
	timer_events.passed_1minute = TRUE;
}

#ifdef NTVDM
ostream & const operator<< (
	ostream & const out,
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
