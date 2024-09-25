//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#ifndef __HEXAGON_HVX__
#define __HEXAGON_HVX__

#define LOG2_HVX_REG_LENGTH 7
#define HVX_REG_LENGTH (1<<LOG2_HVX_REG_LENGTH)

#define LOG2_SIZEOF_BYTE 0
#define SIZEOF_BYTE (1<<LOG2_SIZEOF_BYTE)

#define LOG2_SIZEOF_HALFWORD 1
#define SIZEOF_HALFWORD (1<<LOG2_SIZEOF_HALFWORD)

#define LOG2_SIZEOF_WORD 2
#define SIZEOF_WORD (1<<LOG2_SIZEOF_WORD)

#define LOG2_SIZEOF_DOUBLEWORD 3
#define SIZEOF_DOUBLEWORD (1<<LOG2_SIZEOF_DOUBLEWORD)

#endif  // __HEXAGON_HVX__
