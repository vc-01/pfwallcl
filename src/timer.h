/* 
 * Copyright (c) 2022 Vladimir Chren
 * All rights reserved.
 * 
 * SPDX-License-Identifier: MIT
 */

#ifndef _TIMER_H
#define _TIMER_H 1

#include "pfbios.h"
#include "common.h"

#ifdef __cplusplus
	#define __CPPARGS ...
#else
	#define __CPPARGS
#endif

class Timer
{
private:
	PFBios::clockspeed_t & const
		clockspeed;

	void
		set_poweroff_ticks(unsigned int);
	static void
		dec_poweroff_ticks();
	void
		set_poweroff_delay_default(void);
	int
		get_poweroff_delay_kbhit_ticks(void);

	void interrupt
		(* int1c_handler_orig_fp)(__CPPARGS);
	void interrupt
		(* int4a_handler_orig_fp)(__CPPARGS);
	static void
		interrupt int1c_handler(__CPPARGS);
	static void
		interrupt int4a_handler(__CPPARGS);

public:
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
	};

	struct timer_events_t
	{
		unsigned int passed_1minute : 1;
		unsigned int passed_10minutes : 1;
		unsigned int passed_halfanhour : 1;
		unsigned int passed_fullhour : 1;
		unsigned int poweroff : 1;
	};

	Timer(
		PFBios::clockspeed_t & const,
		struct time_digits_t const & const);
	~Timer();

	void
		set_clockspeed_normal(void);
	void
		set_clockspeed_fast(void);
	void
		set_poweroff_delay_minutes(unsigned int);
	void
		set_poweroff_delay_kbhit(void);
	void
		set_poweroff_delay_minkbhit(void);
	struct Timer::timer_events_t
		eval_events(struct time_digits_t const & const time_digits);
	unsigned int
		get_reset_poweroff_event(void);
	unsigned int
		get_reset_passed1minute_event(void);
	struct timer_events_t
		get_events(void);
	void
		time_events_reset(void);
};

#ifdef NTVDM
#include <iostream.h>
	ostream & const
		operator<< (ostream & const, Timer::time_digits_t const & const);
#endif

#endif
