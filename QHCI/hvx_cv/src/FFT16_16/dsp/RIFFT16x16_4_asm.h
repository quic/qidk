//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#ifndef RIFFT16x16_4_ASM_H
#define RIFFT16x16_4_ASM_H

//FFT_16x16
typedef int16_t real_t;  // <= 11-bit signed integer
typedef struct {
    real_t r;
    real_t i;
} complex_t;

#ifdef __cplusplus
extern "C"
{
#endif

void RIFFT16x16_4_asm(real_t *pOutput, const uint16_t nOutputStride,complex_t *pInput, const uint16_t nInputStride,  int16 * buf_debug); 


#ifdef __cplusplus
}
#endif

#endif    // RIFFT16x16_4_ASM_H
