/*
 * Copyright (c) 2022-2023 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "pfbios.h"

#include <stdlib.h>
#include <dos.h>

#ifdef NTVDM
#include <graphics.h>
#include <iostream.h>
#endif

#define BIOS_VIDEO_SERVICE 0x10
#define INT10_SETMODE 0x00

#define BIOS_TIME_SERVICE 1Ah
#define READ_RTC_TIME 02h

#define MSGBOX_XPOS 1
#define MSGBOX_YPOS 2
/*
 * | | TEXT.TEXT.TEXT.TEXT.TEXT.TEXT.TEXT | |
 * | | |                                | | |
 * | | |                                | | |
 * Left Screen Border                   | | |
 *   Left Double Line (MSGBOX_XPOS)     | | |
 *     Start Of Text Message            | | |
 *                                      | | |
 *                   End Of Text Message  | |
 *                       Right Double Line  |
 *                        Right Screen Border
 */
char const * const
    PFBios::msg_clockspeed_fast =
            "Clock Speed Change"
            "\0"
    //   | | TEXT.TEXT.TEXT.TEXT.TEXT.TEXT.TEXT | |
            "Clock Speed now in FAST mode."
            "\0";
char const * const
    PFBios::msg_clockspeed_normal =
            "Clock Speed Change"
            "\0"
    //   | | TEXT.TEXT.TEXT.TEXT.TEXT.TEXT.TEXT | |
            "Clock Speed now in NORMAL mode."
            "\0";
char const * const
    PFBios::msg_poweroff_delay_override_deact =
            "Power Off Delay"
            "\0"
    //   | | TEXT.TEXT.TEXT.TEXT.TEXT.TEXT.TEXT | |
            "P-off delay override deactivated."
            "\0";
char const * const
    PFBios::msg_poweroff_delay_1h =
            "Power Off Delayed"
            "\0"
    //   | | TEXT.TEXT.TEXT.TEXT.TEXT.TEXT.TEXT | |
            "Will power off in 1 hour."
            "\0";
#define MSG_POWEROFF_DELAY_N_OFFS 36
char * const
    PFBios::msg_poweroff_delay_h =
            "Power Off Delayed"
            "\0"
    //   | | TEXT.TEXT.TEXT.TEXT.TEXT.TEXT.TEXT | |
            "Will power off in N hours."
            "\0";
char const * const
    PFBios::msg_err_wrong_machine =
            "Incompatible system.$";
char const * const
    PFBios::msg_err_bios_ver =
            "Unsupported BIOS version. Try with argument 'untested'.$";

const byte_t tone_code[] = {
        0x30,  //  D#5  622.3 Hz
        0x31,  //  E-5  659.3 Hz
        0x32,  //  F-5  698.5 Hz
        0x33,  //  F#5  740.0 Hz
        0x34,  //  G-5  784.0 Hz
        0x35,  //  G#5  830.6 Hz
        0x36,  //  A-5  880.6 Hz
        0x37,  //  A#5  932.3 Hz
        0x38,  //  B-5  987.8 Hz
        0x39,  //  C-6  1046.5 Hz
        0x3A,  //  C#6  1108.7 Hz
        0x29,  //  D-6  1174.7 Hz
        0x3B,  //  D#6  1244.5 Hz
        0x3C,  //  E-6  1318.5 Hz
        0x3D,  //  F-6  1396.9 Hz
        0x0E,  //  F#6  1480.0 Hz
        0x3E,  //  G-6  1568.9 Hz
        0x2C,  //  G#6  1661.2 Hz
        0x3F,  //  A-6  1760.0 Hz
        0x04,  //  A#6  1864.7 Hz
        0x05,  //  B-6  1975.5 Hz
        0x25,  //  C-7  2093.0 Hz
        0x2F,  //  C#7  2217.5 Hz
        0x06,  //  D-7  2349.3 Hz
        0x07,  //  D#7  2489.0 Hz
};

#pragma warn -rvl
int
    PFBios::check_machinetype(void)
{
#ifndef NTVDM
    // check presence of interrupt handler 0x61
    asm {
        mov ax, 0x3561
        push es
        int 0x21
        mov bx, es
        pop es
        mov ax, 1
        or bx, bx
        jz end
    }
#endif
    asm {
        mov ax, 0
    }
    end:
}
#pragma warn +rvl

#pragma warn -rvl
int
    PFBios::check_biosver(void)
{
    asm {
        push ds
        push bx
        push dx
    }
#ifndef NTVDM
    asm {
        // Int 61h, Fn 2Ch - Get Bios version number
        mov ah, 0x2c
        int 0x61

        mov dx, 0e000h  // set DS manually, BIOS bug?
        mov ds, dx

        mov ah, 2

        cmp byte ptr ds:[bx], '1'
        jne end
        inc bx

        cmp byte ptr ds:[bx], '.'
        jne end
        inc bx

        cmp byte ptr ds:[bx], '0'
        jne end
        inc bx

        cmp byte ptr ds:[bx], '5'
        jne end
        inc bx

        cmp byte ptr ds:[bx], '2'
        jne end
    }
#endif
    asm {
        mov ax, 0
    }
    end:
    asm {
        pop dx
        pop bx
        pop ds
    }
}
#pragma warn +rvl

void
    PFBios::init_int61h(void)
{
#ifndef NTVDM
    // Int 61h, Fn 00h - Service Initialization
    struct REGPACK regpack;
    regpack.r_ax = 0;
    intr (0x61, &regpack);
#endif
}

void PFBios::set_videomode(byte_t mode)
{
#ifndef NTVDM
    union REGS regs;
    regs.h.ah = INT10_SETMODE;
    regs.h.al = mode;
    int86(BIOS_VIDEO_SERVICE, &regs, &regs);
#else // #ifdef NTVDM
    if (mode == VIDMODE_CGA640x200BW)
    {
        char pathtodriver[] = "C:\\BORLANDC\\BGI";
        int gdriver = CGA;
        int gmode = CGAHI;
        initgraph(&gdriver, &gmode, pathtodriver);
    }
    if (mode == VIDMODE_MDATEXT80x25)
    {
        closegraph();
    }
#endif
}

void PFBios::read_rtc_time(
    byte_t & const hour_tens,
    byte_t & const hour_ones,
    byte_t & const minute_tens,
    byte_t & const minute_ones)
{
    asm {
        push ax
        push bx

        /*
            02H ▌AT▐ read the time from the non-volatile (CMOS) real-time clock
            Output: CH = hours in BCD   (Example: CX = 1243H = 12:43)
                    CL = minutes in BCD
                    DH = seconds in BCD
            Output: CF = CY = 1 if clock not operating
        */
        mov ah, READ_RTC_TIME
        int BIOS_TIME_SERVICE

        mov al, ch      // copy hours to al
        mov ah, ch      // copy hours to ah
        and ax, 0f00fh  // ah = hour_tens << 4, al = hour_ones
        shr ah, 4       // ah = hour_tens

        mov bx, word ptr hour_tens
        mov byte ptr [bx], ah
        mov bx, word ptr hour_ones
        mov byte ptr [bx], al

        mov al, cl      // copy minutes to al
        mov ah, cl      // copy minutes to ah
        and ax, 0f00fh  // ch = minute_tens << 4, cl = minute_ones
        shr ah, 4       // ch = minute_tens

        mov bx, word ptr minute_tens
        mov byte ptr [bx], ah
        mov bx, word ptr minute_ones
        mov byte ptr [bx], al

        pop bx
        pop ax
    }
}

void PFBios::reset_rtc_alarm(void)
{
    union REGS regs;
    regs.h.ah = 0x07;               // reset RTC alarm (not implemented in Dosbox)
    int86 (0x1a, &regs, &regs);     // BIOS Timer/Clock Service
}

void PFBios::set_rtc_alarm(
    unsigned int hour,
    unsigned int minute)
{
#ifdef NTVDM
    cout.fill('0');
    cout.width(2);
    cout << "PFBios: Set RTC alarm: "
        << hour
        << ":";
    cout.width(2);
    cout
        << minute
        << "\n";
#endif
    union REGS regs;
    regs.h.ch = dec2bcd(hour);      // hours (BCD)
    regs.h.cl = dec2bcd(minute);    // minutes (BCD)
    regs.h.dh = 0;                  // seconds (BCD)
    regs.h.ah = 0x06;               // set RTC alarm (not implemented in Dosbox)
    int86 (0x1a, &regs, &regs);     // BIOS Timer/Clock Service
}

void PFBios::beep_rndtone(void)
{
#ifndef NTVDM
    // Int 61h, Fn 16h - Melody Tone Generator
    union REGS regs;
    regs.h.ah = 0x16;
    regs.x.cx = 20;                 // Length of tone in 10 ms intervals
    regs.h.dl = rand() % sizeof(tone_code) / 2 + sizeof(tone_code) / 2; // Rand tone code
    int86 (0x61, &regs, &regs);

#else // #ifdef NTVDM
    sound(rand() % 7000);
    delay(100);
    nosound();

#endif
}

#ifdef NTVDM
#pragma argsused
#endif
void PFBios::set_clockspeed(PFBios::clockspeed_t clockspeed)
{
#ifndef NTVDM
    // Int 61h, Fn 1Eh - Get/Set Clock Tick Speed
    struct REGPACK regpack;
    regpack.r_ax = 0x1e << 8 | 1;
    regpack.r_bx = clockspeed;
    intr (0x61, &regpack);
#endif
}

PFBios::clockspeed_t PFBios::get_clockspeed(void)
{
#ifndef NTVDM
    // Int 61h, Fn 1Eh - Get/Set Clock Tick Speed
    struct REGPACK regpack;
    regpack.r_ax = 0x1e << 8 | 0;
    intr (0x61, &regpack);

    if (regpack.r_bx == PFBios::clockspeed_normal)
        return PFBios::clockspeed_normal;
    else if (regpack.r_bx == PFBios::clockspeed_fast)
        return PFBios::clockspeed_fast;

#endif
    return PFBios::clockspeed_normal;
}

#ifdef NTVDM
#pragma argsused
#endif
void
    PFBios::set_cursor_mode(byte_t mode)
{
#ifndef NTVDM
    // Int 60h, Fn 0Fh - Get/Set Cursor Mode
    struct REGPACK regpack;
    regpack.r_bx = mode;                        // BL - New Cursor Mode
    regpack.r_ax = 0x0f << BITS_PER_BYTE | 1;   // AL:1 - Set Mode
    intr (0x61, &regpack);
#endif
}

void
    PFBios::show_message_box(
        char const * msg)
{
#ifndef NTVDM
    set_videomode(VIDMODE_MDATEXT80x25);

    // Int 60h, Fn 12h - Show message box
    struct REGPACK regpack;
    regpack.r_bx =
        0 << BITS_PER_BYTE | 0; // BH - Video page number
    regpack.r_dx =
        MSGBOX_YPOS << BITS_PER_BYTE |
        MSGBOX_XPOS;            // DH - Y position (max 8), DL - X position (max 40)
    regpack.r_ds = FP_SEG(msg);
    regpack.r_si = FP_OFF(msg);
    regpack.r_ax = 0x12 << BITS_PER_BYTE;
    intr (0x60, &regpack);

    asm {
        // wait for keypress
        mov ah, 0
        int 16h
    }

    set_videomode(VIDMODE_CGA640x200BW);
#else // #ifdef NTVDM
    cout
        << "PFBios: "
        << msg              // message title
        << ": ";
    while (*msg++ != NULL); // move to the next string
    cout
        << msg              // message body
        << endl;
#endif
}

void
    PFBios::show_message_box_poweroff_delay_h(
        unsigned int intnum)
{
    msg_poweroff_delay_h[MSG_POWEROFF_DELAY_N_OFFS] = '0' + intnum;
    show_message_box(msg_poweroff_delay_h);
}

void
    PFBios::show_message_earlystage(
        char const * const msg)
{
    // Int 21h, Fn 09h - Print String
    struct REGPACK regpack;
    regpack.r_ax = 0x09 << BITS_PER_BYTE;
    regpack.r_ds = FP_SEG (msg);
    regpack.r_dx = FP_OFF (msg);
    intr (0x21, &regpack);
}

void PFBios::poweroff(void)
{
#ifndef NTVDM
    // Int 61h, Fn 2Dh - Turn System Off
    struct REGPACK regpack;
    regpack.r_ax = 0x2d << 8 | 0;
    intr (0x61, &regpack);
#else
    cout
        << "PFBios: Power off now.\n";
#endif
}

unsigned char PFBios::dec2bcd(unsigned int num)
{
    return ((num / 10) << 4) | (num % 10);
}
