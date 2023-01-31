/*
 * Copyright (c) 2022-2023 Vladimir Chren
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _COMMON_H
#define _COMMON_H 1

#define TRUE 1
#define FALSE 0
#define RET_SUCCESS 0
#define RET_FAILURE -1
#define BITS_PER_BYTE 8
#define BITS_PER_WORD 16
#define BITS_PER_LONG 32
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define MAX(a,b) ((a)>(b) ? (a) : (b))

#define MIN_POFF_DELAY_ONKBHIT_MINUTES 4
#define DEFAULT_POFF_DELAY_ONKBHIT_MINUTES 10

typedef unsigned char byte_t;

#endif
