/*
 * Copyright (c) 2022-2023 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "dgclock.h"
#include "common.h"

#define WE_USE_TEN_DIGITS 10

struct digits_pixeldata_t {
    union {
        byte_t
            arr_b
                [WE_USE_TEN_DIGITS]
                [FNTDATA_HEIGHT]
                [FNTDATA_DIGIT_WIDTH_B];
        unsigned int
            arr_w
                [WE_USE_TEN_DIGITS]
                [FNTDATA_HEIGHT]
                [FNTDATA_DIGIT_WIDTH_W];
        unsigned long
            arr_dw
                [WE_USE_TEN_DIGITS]
                [FNTDATA_HEIGHT];
    };
};

extern "C" {
    extern struct digits_pixeldata_t const
        digits_pixeldata_laligned;

    extern byte_t const
        colon_pixeldata[FNTDATA_HEIGHT];
}

struct digits_pixeldata_t
    digits_pixeldata_raligned;

int const
    DgClock::offs_x_clockdigit_static[] = {
        0,
        1 * FNTDATA_DIGIT_WIDTH_B,
        2 * FNTDATA_DIGIT_WIDTH_B + FNTDATA_COLON_WIDTH_B,
        3 * FNTDATA_DIGIT_WIDTH_B + FNTDATA_COLON_WIDTH_B,
};

DgClock::DgClock(
    Graph::window_arrangement_t const window_arrangement) :
    initial_y_offs(GRAPH_Y_OFFS)
{
    fnt_prep_raligned();
    set_window_arrangement(window_arrangement);
}

void DgClock::fnt_prep_raligned()
{
    union REGS regs;

    for (int digit = 0; digit < WE_USE_TEN_DIGITS; digit++)
    {
        for (int fntdata_row = 0; fntdata_row < FNTDATA_HEIGHT; fntdata_row++)
        {
            if (digit == 1 )
                    regs.h.cl = 10; // digit '1' is shifted by 10 bits
            else
                    regs.h.cl = 4;  // all others by 4 bits

            regs.h.dh = digits_pixeldata_laligned.arr_b[digit][fntdata_row][0];
            regs.h.dl = digits_pixeldata_laligned.arr_b[digit][fntdata_row][1];
            regs.h.ah = digits_pixeldata_laligned.arr_b[digit][fntdata_row][2];
            regs.h.al = digits_pixeldata_laligned.arr_b[digit][fntdata_row][3];

            asm {
                push ax
                push bx
                push dx
                push cx

                mov dx, word ptr [ regs.x.dx ]
                mov ax, word ptr [ regs.x.ax ]
                mov cl, byte ptr [ regs.h.cl ]

                // bit shift right (optimized), taken from BC++3.1 built-in library
                mov bx,dx
                shr ax,cl
                shr dx,cl
                neg cl
                add cl,10h
                shl bx,cl
                or  ax,bx

                mov word ptr [ regs.x.dx ], dx
                mov word ptr [ regs.x.ax ], ax

                pop cx
                pop dx
                pop bx
                pop ax
            }

            digits_pixeldata_raligned.arr_b[digit][fntdata_row][0] = regs.h.dh;
            digits_pixeldata_raligned.arr_b[digit][fntdata_row][1] = regs.h.dl;
            digits_pixeldata_raligned.arr_b[digit][fntdata_row][2] = regs.h.ah;
            digits_pixeldata_raligned.arr_b[digit][fntdata_row][3] = regs.h.al;
        }
    }
}

void DgClock::set_window_arrangement(
    Graph::window_arrangement_t const window_arrangement)
{
    if (window_arrangement == Graph::DGCLOCK_LEFT_ANIM_RIGHT)
    {
        initial_x_offs = 0;
    }
    else if (window_arrangement == Graph::DGCLOCK_RIGHT_ANIM_LEFT)
    {
        initial_x_offs = DGCLOCK_X_OFFS_MAX;
    }
}

void DgClock::draw(Timer::time_digits_t const & time_digits)
{
    for (int clockdigit = 0; clockdigit < 4; clockdigit++)
    {
        int offs_x =
            initial_x_offs / 8 +
            offs_x_clockdigit_static[clockdigit];

        for (int fntdata_row = 0; fntdata_row < FNTDATA_HEIGHT; fntdata_row++)
        {
            int offs_y =
                initial_y_offs +
                fntdata_row;

            if (clockdigit == 0 && time_digits.digit_arr[clockdigit] == 0) {
                // empty (fill with 0's) if hour_tens == 0
#ifndef NTVDM
                *(unsigned long far *)
                    &(*Graph::vram_cga_evenscanlines.ptr_union.arr_b)[offs_y][offs_x] =
                        0;
#else // #ifdef NTVDM
                offs_y /= 2;

                *(unsigned long far *)
                    &(*Graph::vram_cga_evenscanlines.ptr_union.arr_b)[offs_y][offs_x] =
                        0;

                *(unsigned long far *)
                    &(*Graph::vram_cga_oddscanlines.ptr_union.arr_b)[offs_y][offs_x] =
                        0;
#endif
                continue;
            }

            if (clockdigit % 2)
            {
#ifndef NTVDM
                *(unsigned long far *)
                    &(*Graph::vram_cga_evenscanlines.ptr_union.arr_b)[offs_y][offs_x] =
                        digits_pixeldata_laligned.arr_dw[time_digits.digit_arr[clockdigit]]
                            [fntdata_row];
#else // #ifdef NTVDM
                offs_y /= 2;

                *(unsigned long far *)
                    &(*Graph::vram_cga_evenscanlines.ptr_union.arr_b)[offs_y][offs_x] =
                        digits_pixeldata_laligned.arr_dw[time_digits.digit_arr[clockdigit]]
                            [fntdata_row];

                *(unsigned long far *)
                    &(*Graph::vram_cga_oddscanlines.ptr_union.arr_b)[offs_y][offs_x] =
                        digits_pixeldata_laligned.arr_dw[time_digits.digit_arr[clockdigit]]
                            [++fntdata_row];
#endif
            }
            else
            {
#ifndef NTVDM
                *(unsigned long far *)
                    &(*Graph::vram_cga_evenscanlines.ptr_union.arr_b)[offs_y][offs_x] =
                        digits_pixeldata_raligned.arr_dw[time_digits.digit_arr[clockdigit]]
                            [fntdata_row];
#else // #ifdef NTVDM
                offs_y /= 2;

                *(unsigned long far *)
                    &(*Graph::vram_cga_evenscanlines.ptr_union.arr_b)[offs_y][offs_x] =
                        digits_pixeldata_raligned.arr_dw[time_digits.digit_arr[clockdigit]]
                            [fntdata_row];

                *(unsigned long far *)
                    &(*Graph::vram_cga_oddscanlines.ptr_union.arr_b)[offs_y][offs_x] =
                        digits_pixeldata_raligned.arr_dw[time_digits.digit_arr[clockdigit]]
                            [++fntdata_row];
#endif
            }
        }
    }

    int offs_x =
        initial_x_offs / 8 +
        2 * FNTDATA_DIGIT_WIDTH_B;

    for (int fntdata_row = 0; fntdata_row < FNTDATA_HEIGHT; fntdata_row++)
    {
        int offs_y =
            initial_y_offs +
            fntdata_row;

#ifndef NTVDM
        (*Graph::vram_cga_evenscanlines.ptr_union.arr_b)[offs_y][offs_x] =
            colon_pixeldata[fntdata_row];

#else // #ifdef NTVDM
        offs_y /= 2;

        (*Graph::vram_cga_evenscanlines.ptr_union.arr_b)[offs_y][offs_x] =
            colon_pixeldata[fntdata_row];

        (*Graph::vram_cga_oddscanlines.ptr_union.arr_b)[offs_y][offs_x] =
            colon_pixeldata[++fntdata_row];
#endif
    }
}
