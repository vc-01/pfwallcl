/* 
 * Copyright (c) 2022 Vladimir Chren
 * All rights reserved.
 * 
 * SPDX-License-Identifier: MIT
 */

#ifdef NTVDM
#include <graphics.h>
#endif
#include <stdlib.h>
#include "graph.h"

struct Graph::vram_cga_evenscanlines_t const
	Graph::vram_cga_evenscanlines;
#ifdef NTVDM
struct Graph::vram_cga_oddscanlines_t  const
	Graph::vram_cga_oddscanlines;
#endif

Graph::Graph(
	window_arrangement_t const window_arrangement) :
	anim_sin_amplmultp (1)
{
	set_window_arrangement(window_arrangement);
}

void Graph::set_window_arrangement(
	window_arrangement_t const window_arrangement)
{
	if (window_arrangement == Graph::DGCLOCK_LEFT_ANIM_RIGHT)
	{
		anim_initial_x_offs = DISPL_XRES - ANIMW_WIDTH;
	}
	else if (window_arrangement == Graph::DGCLOCK_RIGHT_ANIM_LEFT)
	{
		anim_initial_x_offs = 0;
	}
}

#include <math.h>
#define ANIM_SIN_NUM_WAVES 2
#define ANIM_SIN_WAVELENGTH (ANIMW_WIDTH / ANIM_SIN_NUM_WAVES)
#define ANIM_SIN_WAVEAMPL (FNTDATA_HEIGHT / 2.)
void Graph::anim_clearwindow(void)
{
	for (int offs_y = GRAPH_Y_OFFS;
		offs_y < (GRAPH_Y_OFFS + ANIMW_HEIGHT);
		offs_y++)
	{
		for (int offs_x = anim_initial_x_offs / BITS_PER_WORD;
			offs_x < (anim_initial_x_offs + ANIMW_WIDTH) / BITS_PER_WORD;
			offs_x++)
		{
#ifndef NTVDM
			(*Graph::vram_cga_evenscanlines.ptr_union.arr_w)[offs_y][offs_x] =
				0;
#else // #ifdef NTVDM
			int offs_y_cga = offs_y / 2;

			(*Graph::vram_cga_evenscanlines.ptr_union.arr_w)[offs_y_cga][offs_x] =
				0;

			(*Graph::vram_cga_oddscanlines.ptr_union.arr_w)[offs_y_cga][offs_x] =
				0;
#endif
		}
	}
}

#ifdef SSHOT
#include <io.h>
int
	Graph::take_screenshot(int fd)
{
	byte_t vram_b;
	int vram_b_ptr = 0;
	int res;

	do
	{
		vram_b =
			*(Graph::vram_cga_evenscanlines.ptr_union.ptr_b + vram_b_ptr);

		res = _write (fd, &vram_b, sizeof vram_b);

		if (res == -1)
			return -1;
		if (res != sizeof vram_b)
			return -2;
	}
	while (vram_b_ptr++ < VRAM_SIZE_B);

	return 0;
}
#endif

void Graph::anim_prep(void)
{
	anim_clearwindow();
	anim_sin_x = anim_initial_x_offs;
	anim_sin_amplmultp = 1;

	/*
	 * Animation consists of three sine waves, of which two
	 * are added to the first, base one, with non-equal wavelengths.
	 * This is giving some basic "entertainment" on
	 * different waves for each animation.
	 */

	/*
	 * Sine wave 2&3 length multiplier mustn't match the sine 1
	 * wave length multiplier which is fixed to '1'.
	 */

	/*
	 * Get wave length multiplier for sine wave 2 from
	 * enumerated array of values of:
	 *    [ 0.5, 1.5, 2, 2.5 ]
	 */
	do
		anim_sin2_wavelength_multp =
			0.5 * ( 1 + rand() % ( ANIM_SIN_NUM_WAVES + 2 ) );
	while (anim_sin2_wavelength_multp == 1);

	/*
	 * Get wave length multiplier for sine wave 3 from
	 * enumerated array of values of:
	 *    [ 0.5, 1.5, 2, 2.5 ]
	 * and not equal to the sine wave 2 length multiplier.
	 */
	do
		anim_sin3_wavelength_multp =
			0.5 * ( 1 + rand() % ( ANIM_SIN_NUM_WAVES + 2 ) );
	while (anim_sin3_wavelength_multp == 1 ||
		anim_sin3_wavelength_multp == anim_sin2_wavelength_multp);

	/*
	 * Sine wave 1 amplitude multiplier is fixed to '0.5',
	 * the other two need to sum up to another '0.5'.
	 */

	/*
	 * Get multiplier for sine wave 2&3 making another '0.5'.
	 */

	/* Get amplitude multiplier for sine wave 2 from an array
	 * of values of:
	 *    [ 0.1, 0.2, 0.3, 0.4 ]
	 * and, for sine wave 3, a complement to 0.5.
	 */
	anim_sin2_waveamplmultp =
		0.1 * ( 1 + rand() % 4 );
	anim_sin3_waveamplmultp =
		0.5 - anim_sin2_waveamplmultp;
}

#define ANIM_SIN_AMPL_HYST 0.6
int Graph::anim_iter_amplmultp_finished(void)
{
	anim_sin_x = anim_initial_x_offs;

	/*
	 * Iterate sine amplitude multiplier over
	 * values of:
	 *    [ 1, 0.9, ..., ANIM_SIN_AMPL_HYST ]
	 */
	if (anim_sin_amplmultp > ANIM_SIN_AMPL_HYST)
	{
		anim_sin_amplmultp -= 0.1;
	}
	else
	{
#ifdef NTVDM
		delay(500);
#endif
		return TRUE;	// finished = TRUE
	}
	return FALSE;		// finished = FALSE
}

int Graph::animate_finished(void)
{
	int anim_y;
	int anim_iter = 9;
	while (
		(anim_sin_x < (anim_initial_x_offs + ANIMW_WIDTH)) &&
		(anim_iter-- > 0))
	{
		anim_y  =
			sin(anim_sin_x * 2. * M_PI / ANIM_SIN_WAVELENGTH) *
			ANIM_SIN_WAVEAMPL * .5;
		anim_y +=
			sin(anim_sin_x * 2. * M_PI / (ANIM_SIN_WAVELENGTH * anim_sin2_wavelength_multp)) *
			ANIM_SIN_WAVEAMPL * anim_sin2_waveamplmultp;
		anim_y +=
			sin(anim_sin_x * 2. * M_PI / (ANIM_SIN_WAVELENGTH * anim_sin3_wavelength_multp)) *
			ANIM_SIN_WAVEAMPL * anim_sin3_waveamplmultp;
		anim_y *=
			anim_sin_amplmultp;
		anim_y +=
			GRAPH_Y_OFFS + ANIM_SIN_WAVEAMPL;

		putpix(anim_sin_x++, anim_y);
	}

	if (anim_sin_x >= (anim_initial_x_offs + ANIMW_WIDTH))
		return anim_iter_amplmultp_finished();
	return FALSE; // finished = FALSE
}

// Routines for Hitachi HD61830 display controller

// source code taken from:
//     http://portfolio.wz.cz/programm/pgm_gfx.htm
void Graph::cls_withpattern(unsigned int pattern)
{
  asm {
	push es
	push cx
	push di

	mov  cx,CGA_VRAM_SEG
	mov  es,cx
#ifndef NTVDM
	mov  cx,VRAM_SIZE_W
#else // #ifdef NTVDM
	mov  cx,VRAM_SIZE_W / 4
#endif
	xor  di,di          // offset VRAM=0
	mov  ax,pattern
	rep  stosw          // rep store AX->ES:[DI] while CX > 0

	pop  di
	pop  cx
	pop  es
  }
}

void Graph::cls_withzigzag(void)
{
  asm {
	push es
	push cx
	push dx
	push di

	mov  cx,CGA_VRAM_SEG
	mov  es,cx
	xor  di,di          // offset VRAM=0
	mov  al,0x11
	mov  ah,0
#ifndef NTVDM
	mov  dx,DISPL_YRES
#else // #ifdef NTVDM
	mov  dx,DISPL_YRES / 4
#endif

  }
loop:
  asm {
	mov  cx,VRAM_ROW_B
  }
loop_row:
  asm {
	rep  stosb          // rep store AL->ES:[DI] while CX > 0
	cmp  ah,ZIGZAG_HEIGHT
	ja   ror_label:
	rol  al,1
	inc  ah
	jmp  nextline_label
  }
ror_label:
  asm {
	ror  al,1
	inc  ah
	cmp  ah,ZIGZAG_HEIGHT * 2
	jb   nextline_label
	mov  ah,0
  }
nextline_label:
  asm {
	dec  dx
	jnz  loop

	pop  di
	pop  dx
	pop  cx
	pop  es
  }
}

// source code taken from:
//     http://portfolio.wz.cz/programm/pgm_gfx.htm
void Graph::putpix(int x, int y)
{
#ifndef NTVDM
	register int offset = x + y * DISPL_XRES;
	int byteoffs = offset % 8;
	int vram_offs = offset >> 3;
	*(Graph::vram_cga_evenscanlines.ptr_union.ptr_b + vram_offs) |=
		0x80 >> byteoffs;
#else // #ifdef NTVDM
	putpixel (x, y, WHITE);
#endif
}

// source code taken from:
//     http://portfolio.wz.cz/programm/pgm_gfx.htm
void Graph::vram_copy()
{
  asm {
	cld
	push ax
	push cx
	push dx
	push bx
	push si
	push di
	push ds
	mov  si,0
	mov  ax,0b000h
	mov  ds,ax
	mov  di,64
	}
   refresh_2:
  asm {
	mov  cx,30
	mov  bx,si
	mov  al,0ah
	mov  dx,8011h
	cli
	out  dx,al
	mov  al,bl
	mov  dx,8010h
	out  dx,al
	sti
	mov  al,0bh
	mov  dx,8011h
	cli
	out  dx,al
	mov  dx,8010h
	mov  al,bh
	and  al,7
	out  dx,al
	sti
	}
   refresh_1:
  asm {
	lodsb
	ror  al,1
	mov  ah,al
	and  ah,136
	ror  al,1
	ror  al,1
	mov  bl,al
	and  bl,68
	or   ah,bl
	ror  al,1
	ror  al,1
	mov  bl,al
	and  bl,34
	or   ah,bl
	ror  al,1
	ror  al,1
	and  al,17
	or   al,ah
	mov  ah,al
	inc  dx
	mov  al,0ch
	cli
	out  dx,al
	mov  al,ah
	mov  dx,8010h
	out  dx,al
	sti
	loop refresh_1
	dec  di
	jnz  refresh_2
	pop  ds
	pop  di
	pop  si
	pop  bx
	pop  dx
	pop  cx
	pop  ax
  }
}
