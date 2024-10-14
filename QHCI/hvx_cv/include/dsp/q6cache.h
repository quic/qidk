//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#ifndef Q6CACHE_H
#define Q6CACHE_H

//Record time to log
#define PROFILING_ON
#pragma GCC diagnostic warning "-Wunused-variable"

//Set loop for fft and div
#define LOOPS 1

#include "hexagon_types.h"

#define  CreateL2pfParam(stride, w, h, dir)   (unsigned long long)HEXAGON_V64_CREATE_H((dir), (stride), (w), (h)) 


static void L2fetch(unsigned int addr, unsigned long long param)
{
    __asm__ __volatile__ ("l2fetch(%0,%1)" : : "r"(addr), "r"(param));
}

static inline void WaitForL2fetch()
{
    int usr;
    do
    {
        __asm__ __volatile__ (
            " %0 = usr " :"=r"(usr)
            );
    } while (usr < 0);
}

#endif
