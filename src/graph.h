/*
 * Copyright (c) 2022-2023 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _GRAPH_H
#define _GRAPH_H 1

#include "fixedp.h"
#include "common.h"

#include <dos.h>

#ifndef NTVDM
#define DISPL_XRES 240
#define DISPL_YRES 64
#else // #ifdef NTVDM
#define DISPL_XRES 640
#define DISPL_YRES 200
#endif

#define VRAM_ROW_B (DISPL_XRES / BITS_PER_BYTE)
#define VRAM_ROW_W (DISPL_XRES / BITS_PER_WORD)
#define VRAM_SIZE_B (DISPL_YRES * VRAM_ROW_B )
#define VRAM_SIZE_W (DISPL_YRES * VRAM_ROW_W )

#define CGA_VRAM_SEG 0xB800
#define CGA_VRAM_EVENLINES_OFFS 0
#define CGA_VRAM_ODDLINES_OFFS 0x2000

#define ZIGZAG_HEIGHT 24

#define FNTDATA_DIGIT_WIDTH 32
#define FNTDATA_DIGIT_WIDTH_B (FNTDATA_DIGIT_WIDTH / BITS_PER_BYTE)
#define FNTDATA_DIGIT_WIDTH_W (FNTDATA_DIGIT_WIDTH / BITS_PER_WORD)
#define FNTDATA_COLON_WIDTH 8
#define FNTDATA_COLON_WIDTH_B (FNTDATA_COLON_WIDTH / BITS_PER_BYTE)
#define FNTDATA_HEIGHT 36

#define DGCLOCK_X_OFFS_MAX (DISPL_XRES - 4 * FNTDATA_DIGIT_WIDTH - FNTDATA_COLON_WIDTH)
#define DGCLOCK_Y_OFFS_MAX (DISPL_YRES - FNTDATA_HEIGHT)

#define GRAPH_Y_OFFS (ZIGZAG_HEIGHT * 2 - FNTDATA_HEIGHT)

#define ANIMW_MARGIN_X BITS_PER_WORD
#define ANIMW_WIDTH (DGCLOCK_X_OFFS_MAX / BITS_PER_WORD * BITS_PER_WORD /* cutting fraction out */ - ANIMW_MARGIN_X)
#define ANIMW_HEIGHT FNTDATA_HEIGHT

class Graph
{
    int
        animw_initial_x_offs,
        animw_x_offset,
        sin_bigamplmultp_tenfold;

    Fixedp
        sin_2_wavelengthmultp,
        sin_3_wavelengthmultp,
        sin_2_waveamplmultp,
        sin_3_waveamplmultp,
        sin_wavelength_fixedp;
        
    Fixedp const
        pi_fixedp;

    void
        anim_clearwindow(void);
    int
        anim_iter_amplmultp_finished(void);
public:
    enum window_arrangement_t {
        DGCLOCK_LEFT_ANIM_RIGHT,
        DGCLOCK_RIGHT_ANIM_LEFT,
    };

    struct vram_cga_evenscanlines_t {
        union ptr_union_t {
            byte_t far *
                ptr_b;
            unsigned int far *
                ptr_w;
            byte_t
                (far * arr_b)[DISPL_YRES][VRAM_ROW_B];
            unsigned int
                (far * arr_w)[DISPL_YRES][VRAM_ROW_W];

            ptr_union_t() :
                ptr_b((byte_t far *)
                    MK_FP (CGA_VRAM_SEG, CGA_VRAM_EVENLINES_OFFS))
                {}
        } ptr_union; // this use is forbidden by the C++ standard, well ...
    };
#ifdef NTVDM
    struct vram_cga_oddscanlines_t {
        union ptr_union_t {
            byte_t far *
                ptr_b;
            unsigned int far *
                ptr_w;
            byte_t
                (far * arr_b)[DISPL_YRES][VRAM_ROW_B];
            unsigned int
                (far * arr_w)[DISPL_YRES][VRAM_ROW_W];

            ptr_union_t() :
                ptr_b((byte_t far *)
                    MK_FP (CGA_VRAM_SEG, CGA_VRAM_ODDLINES_OFFS))
                {}
        } ptr_union; // this use is forbidden by the C++ standard, well ...
    };
#endif
    static struct vram_cga_evenscanlines_t const
        vram_cga_evenscanlines;
#ifdef NTVDM
    static struct vram_cga_oddscanlines_t const
        vram_cga_oddscanlines;
#endif

    Graph(window_arrangement_t const);

    void
        cls_withpattern(unsigned int);
    void
        cls_withzigzag(void);
    void
        set_window_arrangement(window_arrangement_t const);
#ifdef SSHOT
    int
        take_screenshot(int);
#endif
    void
        anim_prep(void);
    int
        animate_finished(void);
    void
        putpix(int, int);
    void
        vram_copy();
};

#endif
