/*
 * Copyright (c) 2022-2023 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "graph.h"

#include <stdlib.h>

#ifdef NTVDM
#include <graphics.h>
#endif

struct Graph::vram_cga_evenscanlines_t const
    Graph::vram_cga_evenscanlines;
#ifdef NTVDM
struct Graph::vram_cga_oddscanlines_t  const
    Graph::vram_cga_oddscanlines;
#endif

Graph::Graph(
    window_arrangement_t const window_arrangement) :
    pi_fixedp (FIXEDP_PI_RAW, TRUE)
{
    set_window_arrangement(window_arrangement);
}

void Graph::set_window_arrangement(
    window_arrangement_t const window_arrangement)
{
    if (window_arrangement == Graph::DGCLOCK_LEFT_ANIM_RIGHT)
    {
        animw_initial_x_offs = DISPL_XRES - ANIMW_WIDTH;
    }
    else if (window_arrangement == Graph::DGCLOCK_RIGHT_ANIM_LEFT)
    {
        animw_initial_x_offs = 0;
    }
}

void Graph::anim_clearwindow(void)
{
    for (int offs_y = GRAPH_Y_OFFS;
        offs_y < (GRAPH_Y_OFFS + ANIMW_HEIGHT);
        offs_y++)
    {
        for (int offs_x = animw_initial_x_offs / BITS_PER_WORD;
            offs_x < (animw_initial_x_offs + ANIMW_WIDTH) / BITS_PER_WORD;
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

#define ANIM_SIN_NUM_WAVES 3
#define ANIM_SIN_WAVEAMPL (FNTDATA_HEIGHT / 2l)
void Graph::anim_prep(void)
{
    anim_clearwindow();
    animw_x_offset = animw_initial_x_offs;
    sin_wavelength_fixedp = Fixedp(2l) * pi_fixedp / (ANIMW_WIDTH / 2l);
    sin_bigamplmultp_tenfold = 10;

    long sin_2_wavelengthmultp_tenfold;
    long sin_3_wavelengthmultp_tenfold;
    long sin_2_waveamplmultp_tenfold;
    long sin_3_waveamplmultp_tenfold;

    /*
     * The animated picture consists of three superposed sine waves.
     *
     * Let wave length of the first one be '1', the other two
     * will differ in multiples of '0.5' in range '0.5' to '2.5'.
     *
     * Length multiplier of one sine wave mustn't match with any other.
     */

    /*
     * Get wave length multiplier for sine wave 2 from
     * enumerated array of values of:
     *    [ 0.5, 1.5, 2, 2.5 ]
     */
    do
        sin_2_wavelengthmultp_tenfold =
            5 * ( 1 + rand() % ( ANIM_SIN_NUM_WAVES + 1 ) );
    while (sin_2_wavelengthmultp_tenfold == 10);

    /*
     * Get wave length multiplier for sine wave 3 from
     * enumerated array of values of:
     *    [ 0.5, 1.5, 2, 2.5 ]
     * and not equal to sine wave 2 length multiplier.
     */
    do
        sin_3_wavelengthmultp_tenfold =
            5 * ( 1 + rand() % ( ANIM_SIN_NUM_WAVES + 1 ) );
    while (sin_3_wavelengthmultp_tenfold == 10 ||
        sin_3_wavelengthmultp_tenfold == sin_2_wavelengthmultp_tenfold);

    /*
     * Sine wave 1 amplitude multiplier is fixed to '0.5',
     * the other two sum up to another '0.5'.
     */

    /*
     * Get amplitude multiplier for sine wave 2 from
     * enumerated array of values of:
     *    [ 0.1, 0.2, 0.3, 0.4 ]
     */
    sin_2_waveamplmultp_tenfold =
        1 + rand() % 4;

    /*
     * Get amplitude multiplier for sine wave 3 as
     * complement to 0.5.
     */
    sin_3_waveamplmultp_tenfold =
        5 - sin_2_waveamplmultp_tenfold;

    sin_2_wavelengthmultp = Fixedp(sin_2_wavelengthmultp_tenfold) / 10l;
    sin_3_wavelengthmultp = Fixedp(sin_3_wavelengthmultp_tenfold) / 10l;
    sin_2_waveamplmultp = Fixedp(sin_2_waveamplmultp_tenfold) / 10l;
    sin_3_waveamplmultp = Fixedp(sin_3_waveamplmultp_tenfold) / 10l;
}

#define ANIM_SIN_AMPL_HYST 6
int Graph::anim_iter_amplmultp_finished(void)
{
    animw_x_offset = animw_initial_x_offs;

    /*
     * Iterate sine amplitude multiplier over
     * values of:
     *    [ 1, 0.9, ..., ANIM_SIN_AMPL_HYST ]
     */
    if (sin_bigamplmultp_tenfold > ANIM_SIN_AMPL_HYST)
    {
        sin_bigamplmultp_tenfold -= 1;
    }
    else
    {
#ifdef NTVDM
        delay(500);
#endif
        return TRUE;    // finished = TRUE
    }
    return FALSE;       // finished = FALSE
}

#ifdef EMUFPU
#include <math.h>
int Graph::animate_finished(void)
{
    double animwin_ypos;
    int anim_iter = 9;
    while (
        (animw_x_offset < (animw_initial_x_offs + ANIMW_WIDTH)) &&
        (anim_iter-- > 0))
    {
        double x_1 = animw_x_offset * 2 * M_PI / (ANIMW_WIDTH / 2);
        double x_2 = x_1 / sin_2_wavelengthmultp.to_double();
        double x_3 = x_1 / sin_3_wavelengthmultp.to_double();

        animwin_ypos  =
            ANIM_SIN_WAVEAMPL * .5 *
            sin(x_1);
        animwin_ypos +=
            ANIM_SIN_WAVEAMPL * sin_2_waveamplmultp.to_double() *
            sin(x_2);
        animwin_ypos +=
            ANIM_SIN_WAVEAMPL * sin_3_waveamplmultp.to_double() *
            sin(x_3);
        animwin_ypos *=
            sin_bigamplmultp_tenfold / 10.;
        animwin_ypos +=
            GRAPH_Y_OFFS + ANIM_SIN_WAVEAMPL;

        putpix(animw_x_offset++, (int)animwin_ypos);
    }

    if (animw_x_offset >= (animw_initial_x_offs + ANIMW_WIDTH))
        return anim_iter_amplmultp_finished(); // finished = TRUE or FALSE
    return FALSE; // finished = FALSE
}
#else // fixed point arithmetic
int Graph::animate_finished(void)
{
    Fixedp animwin_ypos;
    int anim_iter = 9;
    while (
        (animw_x_offset < (animw_initial_x_offs + ANIMW_WIDTH)) &&
        (anim_iter-- > 0))
    {
        Fixedp x_1 = Fixedp(animw_x_offset) * sin_wavelength_fixedp;
        Fixedp x_2 = x_1 / sin_2_wavelengthmultp;
        Fixedp x_3 = x_1 / sin_3_wavelengthmultp;
        Fixedp sin_bigamplmultp = Fixedp(sin_bigamplmultp_tenfold) / 10l;

        animwin_ypos =
            Fixedp(ANIM_SIN_WAVEAMPL) / 2l *
            Fixedp::quasisin_fixedp(x_1);
        animwin_ypos +=
            (Fixedp(ANIM_SIN_WAVEAMPL) * sin_2_waveamplmultp *
            Fixedp::quasisin_fixedp(x_2));
        animwin_ypos +=
            (Fixedp(ANIM_SIN_WAVEAMPL) * sin_3_waveamplmultp *
            Fixedp::quasisin_fixedp(x_3));
        animwin_ypos *=
            sin_bigamplmultp;
        animwin_ypos +=
            GRAPH_Y_OFFS + ANIM_SIN_WAVEAMPL;

        putpix(animw_x_offset++, animwin_ypos.to_integer());
    }

    if (animw_x_offset >= (animw_initial_x_offs + ANIMW_WIDTH))
        return anim_iter_amplmultp_finished(); // finished = TRUE or FALSE
    return FALSE; // finished = FALSE
}
#endif

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
