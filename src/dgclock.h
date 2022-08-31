/* 
 * Copyright (c) 2022 Vladimir Chren
 * All rights reserved.
 * 
 * SPDX-License-Identifier: MIT
 */

#ifndef _DGCLOCK_H
#define _DGCLOCK_H 1

#include "graph.h"
#include "timer.h"

class DgClock
{
	int
		initial_x_offs,
		initial_y_offs;
	static int const
		offs_x_clockdigit_static[];
	void
		fnt_prep_raligned();
public:
	DgClock(
		Graph::window_arrangement_t const);

	void
		set_window_arrangement(
			Graph::window_arrangement_t const);
	void
		draw(
			Timer::time_digits_t const &);
};

#endif
