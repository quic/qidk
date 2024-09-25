//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include "QhciBase.hpp"
#include <math.h>

#define FEATURE_NAME "FFT16_16"

// Step0. Define reference code here
typedef int16_t real_t; // <= 11-bit signed integer
typedef struct
{
    real_t r;
    real_t i;
} complex_t;

#define LOG2_HVX_REG_LENGTH 7
#define HVX_REG_LENGTH (1 << LOG2_HVX_REG_LENGTH)
#define RIGHT_SHIFT_ROUND(v, s) (((v) + (1 << ((s)-1))) >> (s))
#define CLIP3(a, b, x) ((x) < (a) ? (a) : (x) > (b) ? (b) \
                                                    : (x))
#define AVG_ROUND(x, y) RIGHT_SHIFT_ROUND((x) + (y), 1)
#define NAVG(x, y) (((x) - (y)) >> 1)

#define Q_FACTOR 15
#define COS_PI_8 30274
#define SIN_PI_8 12540
#define COS_PI_4 23170
#define MPY(x, y) RIGHT_SHIFT_ROUND((x) * (y), Q_FACTOR)
#define SAT(x) CLIP3(INT16_MIN, INT16_MAX, x)
#define RFFT_LENGTH (16 / 2 + 1)

static void RFFT1x4(complex_t *pOutput, const real_t *pInput, const uint16_t nOffset)
{
    real_t anStage1[4];

    // input: 10-bit unsigned integer
    // output: 12-bit signed integer
    anStage1[0] = pInput[0 * 4 * nOffset] + pInput[2 * 4 * nOffset];
    anStage1[1] = pInput[0 * 4 * nOffset] - pInput[2 * 4 * nOffset];
    anStage1[2] = pInput[1 * 4 * nOffset] + pInput[3 * 4 * nOffset];
    anStage1[3] = pInput[1 * 4 * nOffset] - pInput[3 * 4 * nOffset];

    // input: 12-bit signed integer
    // output: 13-bit signed integer
    pOutput[0].r = anStage1[0] + anStage1[2];
    pOutput[1].r = anStage1[1];
    pOutput[1].i = -anStage1[3];
    pOutput[2].r = anStage1[0] - anStage1[2];
}
int cont = 0;
static void RFFT1x16(complex_t *pOutput, const real_t *pInput, const uint16_t nOffset)
{
    complex_t anStage1[4 * 3];
    complex_t anTmp[4 * 4];
    complex_t anStage2[4 * 4];

    for (int32_t i = 0; i < 4; i++)
    {
        // input: 10-bit unsigned integer
        // output: 13-bit signed integer
        RFFT1x4(&anStage1[i * 3], &pInput[i * nOffset], nOffset);
    }

    // input: 13-bit signed integer
    // output: 14-bit signed integer
    anStage2[0 * 4 + 0].r = anStage1[0 * 3 + 0].r + anStage1[2 * 3 + 0].r;
    anStage2[1 * 4 + 0].r = anStage1[0 * 3 + 0].r - anStage1[2 * 3 + 0].r;
    anStage2[2 * 4 + 0].r = anStage1[1 * 3 + 0].r + anStage1[3 * 3 + 0].r;
    anStage2[3 * 4 + 0].r = anStage1[1 * 3 + 0].r - anStage1[3 * 3 + 0].r;

    // input: 14-bit signed integer
    // output: 15-bit signed integer
    pOutput[0 * 4 + 0].r = anStage2[0 * 4 + 0].r + anStage2[2 * 4 + 0].r;
    pOutput[0 * 4 + 0].i = 0;
    pOutput[1 * 4 + 0].r = anStage2[1 * 4 + 0].r;
    pOutput[1 * 4 + 0].i = -anStage2[3 * 4 + 0].r;
    pOutput[2 * 4 + 0].r = anStage2[0 * 4 + 0].r - anStage2[2 * 4 + 0].r;
    pOutput[2 * 4 + 0].i = 0;

    // input: 13-bit signed integer
    // output: 13-bit signed integer
    anTmp[0 * 4 + 1].r = anStage1[0 * 3 + 1].r; //
    anTmp[0 * 4 + 1].i = anStage1[0 * 3 + 1].i;
    anTmp[1 * 4 + 1].r = MPY(COS_PI_8, anStage1[1 * 3 + 1].r) + MPY(SIN_PI_8, anStage1[1 * 3 + 1].i);
    anTmp[1 * 4 + 1].i = -MPY(SIN_PI_8, anStage1[1 * 3 + 1].r) + MPY(COS_PI_8, anStage1[1 * 3 + 1].i);
    anTmp[2 * 4 + 1].r = MPY(COS_PI_4, anStage1[2 * 3 + 1].r) + MPY(COS_PI_4, anStage1[2 * 3 + 1].i); //
    anTmp[2 * 4 + 1].i = -MPY(COS_PI_4, anStage1[2 * 3 + 1].r) + MPY(COS_PI_4, anStage1[2 * 3 + 1].i);
    anTmp[3 * 4 + 1].r = MPY(SIN_PI_8, anStage1[3 * 3 + 1].r) + MPY(COS_PI_8, anStage1[3 * 3 + 1].i);
    anTmp[3 * 4 + 1].i = -MPY(COS_PI_8, anStage1[3 * 3 + 1].r) + MPY(SIN_PI_8, anStage1[3 * 3 + 1].i);

    // input: 13-bit signed integer
    // output: 14-bit signed integer
    anStage2[0 * 4 + 1].r = anTmp[0 * 4 + 1].r + anTmp[2 * 4 + 1].r;
    anStage2[0 * 4 + 1].i = anTmp[0 * 4 + 1].i + anTmp[2 * 4 + 1].i;
    anStage2[1 * 4 + 1].r = anTmp[0 * 4 + 1].r - anTmp[2 * 4 + 1].r; //
    anStage2[1 * 4 + 1].i = anTmp[0 * 4 + 1].i - anTmp[2 * 4 + 1].i;
    anStage2[2 * 4 + 1].r = anTmp[1 * 4 + 1].r + anTmp[3 * 4 + 1].r;
    anStage2[2 * 4 + 1].i = anTmp[1 * 4 + 1].i + anTmp[3 * 4 + 1].i;
    anStage2[3 * 4 + 1].r = anTmp[1 * 4 + 1].r - anTmp[3 * 4 + 1].r;
    anStage2[3 * 4 + 1].i = anTmp[1 * 4 + 1].i - anTmp[3 * 4 + 1].i; //

    // input: 14-bit signed integer
    // output: 15-bit signed integer
    pOutput[0 * 4 + 1].r = anStage2[0 * 4 + 1].r + anStage2[2 * 4 + 1].r; //
    pOutput[0 * 4 + 1].i = anStage2[0 * 4 + 1].i + anStage2[2 * 4 + 1].i;
    pOutput[1 * 4 + 1].r = anStage2[1 * 4 + 1].r + anStage2[3 * 4 + 1].i; //
    pOutput[1 * 4 + 1].i = anStage2[1 * 4 + 1].i - anStage2[3 * 4 + 1].r;

    // input: 13-bit signed integer
    // output: 13-bit signed integer
    anTmp[0 * 4 + 2].r = anStage1[0 * 3 + 2].r;
    anTmp[1 * 4 + 2].r = MPY(COS_PI_4, anStage1[1 * 3 + 2].r);
    anTmp[1 * 4 + 2].i = anTmp[1 * 4 + 2].r;
    anTmp[2 * 4 + 2].i = anStage1[2 * 3 + 2].r;
    anTmp[3 * 4 + 2].r = MPY(COS_PI_4, anStage1[3 * 3 + 2].r);
    anTmp[3 * 4 + 2].i = anTmp[3 * 4 + 2].r;

    // input: 13-bit signed integer
    // output: 14-bit signed integer
    anStage2[0 * 4 + 2].r = anTmp[0 * 4 + 2].r;
    anStage2[0 * 4 + 2].i = anTmp[2 * 4 + 2].i;
    anStage2[1 * 4 + 2].r = anTmp[0 * 4 + 2].r;
    anStage2[1 * 4 + 2].i = anTmp[2 * 4 + 2].i;
    anStage2[2 * 4 + 2].r = anTmp[1 * 4 + 2].r - anTmp[3 * 4 + 2].r;
    anStage2[2 * 4 + 2].i = anTmp[1 * 4 + 2].i + anTmp[3 * 4 + 2].i;
    anStage2[3 * 4 + 2].r = anTmp[1 * 4 + 2].r + anTmp[3 * 4 + 2].r;
    anStage2[3 * 4 + 2].i = -anTmp[1 * 4 + 2].i + anTmp[3 * 4 + 2].i;
    //	printf("anStage2[1*4+2].r %d= %d\n",cont,anStage2[1*4+2].r);
    // printf("anStage2[3*4+2].i %d= %d\n",cont,anStage2[3*4+2].i);

    // input: 14-bit signed integer
    // output: 15-bit signed integer
    pOutput[0 * 4 + 2].r = anStage2[0 * 4 + 2].r + anStage2[2 * 4 + 2].r;
    pOutput[0 * 4 + 2].i = -anStage2[0 * 4 + 2].i - anStage2[2 * 4 + 2].i;
    pOutput[1 * 4 + 2].r = anStage2[1 * 4 + 2].r + anStage2[3 * 4 + 2].i; //
    pOutput[1 * 4 + 2].i = anStage2[1 * 4 + 2].i - anStage2[3 * 4 + 2].r;

    // input: 14-bit signed integer
    // output: 15-bit signed integer
    pOutput[0 * 4 + 3].r = anStage2[1 * 4 + 1].r - anStage2[3 * 4 + 1].i; //
    pOutput[0 * 4 + 3].i = -anStage2[1 * 4 + 1].i - anStage2[3 * 4 + 1].r;
    pOutput[1 * 4 + 3].r = anStage2[0 * 4 + 1].r - anStage2[2 * 4 + 1].r; //
    pOutput[1 * 4 + 3].i = -anStage2[0 * 4 + 1].i + anStage2[2 * 4 + 1].i;
}

static void CFFT4x1(complex_t *pOutput, const complex_t *pInput)
{
    complex_t anStage1[4];

    // input: 15-bit signed integer
    // output: 16-bit signed integer
    anStage1[0].r = pInput[0 * 4 * RFFT_LENGTH].r + pInput[2 * 4 * RFFT_LENGTH].r;
    anStage1[0].i = pInput[0 * 4 * RFFT_LENGTH].i + pInput[2 * 4 * RFFT_LENGTH].i; // D0i
    anStage1[1].r = pInput[0 * 4 * RFFT_LENGTH].r - pInput[2 * 4 * RFFT_LENGTH].r;
    anStage1[1].i = pInput[0 * 4 * RFFT_LENGTH].i - pInput[2 * 4 * RFFT_LENGTH].i;
    anStage1[2].r = pInput[1 * 4 * RFFT_LENGTH].r + pInput[3 * 4 * RFFT_LENGTH].r;
    anStage1[2].i = pInput[1 * 4 * RFFT_LENGTH].i + pInput[3 * 4 * RFFT_LENGTH].i;
    anStage1[3].r = pInput[1 * 4 * RFFT_LENGTH].r - pInput[3 * 4 * RFFT_LENGTH].r;
    anStage1[3].i = pInput[1 * 4 * RFFT_LENGTH].i - pInput[3 * 4 * RFFT_LENGTH].i;

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    pOutput[0].r = AVG_ROUND(anStage1[0].r, anStage1[2].r); // E0r
    pOutput[0].i = AVG_ROUND(anStage1[0].i, anStage1[2].i);
    pOutput[1].r = AVG_ROUND(anStage1[1].r, anStage1[3].i);
    pOutput[1].i = NAVG(anStage1[1].i, anStage1[3].r);
    pOutput[2].r = NAVG(anStage1[0].r, anStage1[2].r);
    pOutput[2].i = NAVG(anStage1[0].i, anStage1[2].i);
    pOutput[3].r = NAVG(anStage1[1].r, anStage1[3].i);
    pOutput[3].i = AVG_ROUND(anStage1[1].i, anStage1[3].r);
}

static void CFFT16x1(complex_t *pOutput, const uint16_t nOutputStride, const complex_t *pInput)
{
    complex_t anStage1[4 * 4];
    complex_t anTmp[4 * 4];
    complex_t anStage2[4 * 4];

    for (int32_t i = 0; i < 4; i++)
    {
        // input: 15-bit signed integer
        // output: 16-bit signed integer
        CFFT4x1(&anStage1[i * 4], &pInput[i * RFFT_LENGTH]);
    }

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    anStage2[0 * 4 + 0].r = AVG_ROUND(anStage1[0 * 4 + 0].r, anStage1[2 * 4 + 0].r); // F00r
    anStage2[0 * 4 + 0].i = AVG_ROUND(anStage1[0 * 4 + 0].i, anStage1[2 * 4 + 0].i);
    anStage2[1 * 4 + 0].r = NAVG(anStage1[0 * 4 + 0].r, anStage1[2 * 4 + 0].r);
    anStage2[1 * 4 + 0].i = NAVG(anStage1[0 * 4 + 0].i, anStage1[2 * 4 + 0].i);
    anStage2[2 * 4 + 0].r = AVG_ROUND(anStage1[1 * 4 + 0].r, anStage1[3 * 4 + 0].r);
    anStage2[2 * 4 + 0].i = AVG_ROUND(anStage1[1 * 4 + 0].i, anStage1[3 * 4 + 0].i);
    anStage2[3 * 4 + 0].r = NAVG(anStage1[1 * 4 + 0].r, anStage1[3 * 4 + 0].r);
    anStage2[3 * 4 + 0].i = NAVG(anStage1[1 * 4 + 0].i, anStage1[3 * 4 + 0].i);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    pOutput[(0 * 4 + 0) * nOutputStride].r = AVG_ROUND(anStage2[0 * 4 + 0].r, anStage2[2 * 4 + 0].r); // G
    pOutput[(0 * 4 + 0) * nOutputStride].i = AVG_ROUND(anStage2[0 * 4 + 0].i, anStage2[2 * 4 + 0].i);
    pOutput[(1 * 4 + 0) * nOutputStride].r = AVG_ROUND(anStage2[1 * 4 + 0].r, anStage2[3 * 4 + 0].i);
    pOutput[(1 * 4 + 0) * nOutputStride].i = NAVG(anStage2[1 * 4 + 0].i, anStage2[3 * 4 + 0].r);
    pOutput[(2 * 4 + 0) * nOutputStride].r = NAVG(anStage2[0 * 4 + 0].r, anStage2[2 * 4 + 0].r);
    pOutput[(2 * 4 + 0) * nOutputStride].i = NAVG(anStage2[0 * 4 + 0].i, anStage2[2 * 4 + 0].i);
    pOutput[(3 * 4 + 0) * nOutputStride].r = NAVG(anStage2[1 * 4 + 0].r, anStage2[3 * 4 + 0].i);
    pOutput[(3 * 4 + 0) * nOutputStride].i = AVG_ROUND(anStage2[1 * 4 + 0].i, anStage2[3 * 4 + 0].r);

    anTmp[0 * 4 + 1].r = anStage1[0 * 4 + 1].r;
    anTmp[0 * 4 + 1].i = anStage1[0 * 4 + 1].i;
    anTmp[1 * 4 + 1].r = MPY(COS_PI_8, anStage1[1 * 4 + 1].r) + MPY(SIN_PI_8, anStage1[1 * 4 + 1].i);
    anTmp[1 * 4 + 1].i = -MPY(SIN_PI_8, anStage1[1 * 4 + 1].r) + MPY(COS_PI_8, anStage1[1 * 4 + 1].i);
    anTmp[2 * 4 + 1].r = MPY(COS_PI_4, anStage1[2 * 4 + 1].r) + MPY(COS_PI_4, anStage1[2 * 4 + 1].i);
    anTmp[2 * 4 + 1].i = -MPY(COS_PI_4, anStage1[2 * 4 + 1].r) + MPY(COS_PI_4, anStage1[2 * 4 + 1].i);
    anTmp[3 * 4 + 1].r = MPY(SIN_PI_8, anStage1[3 * 4 + 1].r) + MPY(COS_PI_8, anStage1[3 * 4 + 1].i);
    anTmp[3 * 4 + 1].i = -MPY(COS_PI_8, anStage1[3 * 4 + 1].r) + MPY(SIN_PI_8, anStage1[3 * 4 + 1].i);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    anStage2[0 * 4 + 1].r = AVG_ROUND(anTmp[0 * 4 + 1].r, anTmp[2 * 4 + 1].r);
    anStage2[0 * 4 + 1].i = AVG_ROUND(anTmp[0 * 4 + 1].i, anTmp[2 * 4 + 1].i);
    anStage2[1 * 4 + 1].r = NAVG(anTmp[0 * 4 + 1].r, anTmp[2 * 4 + 1].r);
    anStage2[1 * 4 + 1].i = NAVG(anTmp[0 * 4 + 1].i, anTmp[2 * 4 + 1].i);
    anStage2[2 * 4 + 1].r = AVG_ROUND(anTmp[1 * 4 + 1].r, anTmp[3 * 4 + 1].r);
    anStage2[2 * 4 + 1].i = AVG_ROUND(anTmp[1 * 4 + 1].i, anTmp[3 * 4 + 1].i);
    anStage2[3 * 4 + 1].r = NAVG(anTmp[1 * 4 + 1].r, anTmp[3 * 4 + 1].r);
    anStage2[3 * 4 + 1].i = NAVG(anTmp[1 * 4 + 1].i, anTmp[3 * 4 + 1].i);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    pOutput[(0 * 4 + 1) * nOutputStride].r = AVG_ROUND(anStage2[0 * 4 + 1].r, anStage2[2 * 4 + 1].r);
    pOutput[(0 * 4 + 1) * nOutputStride].i = AVG_ROUND(anStage2[0 * 4 + 1].i, anStage2[2 * 4 + 1].i);
    pOutput[(1 * 4 + 1) * nOutputStride].r = AVG_ROUND(anStage2[1 * 4 + 1].r, anStage2[3 * 4 + 1].i);
    pOutput[(1 * 4 + 1) * nOutputStride].i = NAVG(anStage2[1 * 4 + 1].i, anStage2[3 * 4 + 1].r);
    pOutput[(2 * 4 + 1) * nOutputStride].r = NAVG(anStage2[0 * 4 + 1].r, anStage2[2 * 4 + 1].r);
    pOutput[(2 * 4 + 1) * nOutputStride].i = NAVG(anStage2[0 * 4 + 1].i, anStage2[2 * 4 + 1].i);
    pOutput[(3 * 4 + 1) * nOutputStride].r = NAVG(anStage2[1 * 4 + 1].r, anStage2[3 * 4 + 1].i);
    pOutput[(3 * 4 + 1) * nOutputStride].i = AVG_ROUND(anStage2[1 * 4 + 1].i, anStage2[3 * 4 + 1].r);

    anTmp[0 * 4 + 2].r = anStage1[0 * 4 + 2].r;
    anTmp[0 * 4 + 2].i = anStage1[0 * 4 + 2].i;
    anTmp[1 * 4 + 2].r = MPY(COS_PI_4, anStage1[1 * 4 + 2].r) + MPY(COS_PI_4, anStage1[1 * 4 + 2].i);
    anTmp[1 * 4 + 2].i = -MPY(COS_PI_4, anStage1[1 * 4 + 2].r) + MPY(COS_PI_4, anStage1[1 * 4 + 2].i);
    anTmp[2 * 4 + 2].r = anStage1[2 * 4 + 2].i;
    anTmp[2 * 4 + 2].i = anStage1[2 * 4 + 2].r;
    anTmp[3 * 4 + 2].r = -MPY(COS_PI_4, anStage1[3 * 4 + 2].r) + MPY(COS_PI_4, anStage1[3 * 4 + 2].i);
    anTmp[3 * 4 + 2].i = MPY(COS_PI_4, anStage1[3 * 4 + 2].r) + MPY(COS_PI_4, anStage1[3 * 4 + 2].i);

    // input: 16-bit signed integer
    // output: 16-bit signed integer

    anStage2[0 * 4 + 2].r = AVG_ROUND(anTmp[0 * 4 + 2].r, anTmp[2 * 4 + 2].r);
    anStage2[0 * 4 + 2].i = NAVG(anTmp[0 * 4 + 2].i, anTmp[2 * 4 + 2].i);
    anStage2[1 * 4 + 2].r = NAVG(anTmp[0 * 4 + 2].r, anTmp[2 * 4 + 2].r);
    anStage2[1 * 4 + 2].i = AVG_ROUND(anTmp[0 * 4 + 2].i, anTmp[2 * 4 + 2].i);
    anStage2[2 * 4 + 2].r = AVG_ROUND(anTmp[1 * 4 + 2].r, anTmp[3 * 4 + 2].r);
    anStage2[2 * 4 + 2].i = NAVG(anTmp[1 * 4 + 2].i, anTmp[3 * 4 + 2].i);
    anStage2[3 * 4 + 2].r = NAVG(anTmp[1 * 4 + 2].r, anTmp[3 * 4 + 2].r);
    anStage2[3 * 4 + 2].i = AVG_ROUND(anTmp[1 * 4 + 2].i, anTmp[3 * 4 + 2].i);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    pOutput[(0 * 4 + 2) * nOutputStride].r = AVG_ROUND(anStage2[0 * 4 + 2].r, anStage2[2 * 4 + 2].r);
    pOutput[(0 * 4 + 2) * nOutputStride].i = AVG_ROUND(anStage2[0 * 4 + 2].i, anStage2[2 * 4 + 2].i);
    pOutput[(1 * 4 + 2) * nOutputStride].r = AVG_ROUND(anStage2[1 * 4 + 2].r, anStage2[3 * 4 + 2].i);
    pOutput[(1 * 4 + 2) * nOutputStride].i = NAVG(anStage2[1 * 4 + 2].i, anStage2[3 * 4 + 2].r);
    pOutput[(2 * 4 + 2) * nOutputStride].r = NAVG(anStage2[0 * 4 + 2].r, anStage2[2 * 4 + 2].r);
    pOutput[(2 * 4 + 2) * nOutputStride].i = NAVG(anStage2[0 * 4 + 2].i, anStage2[2 * 4 + 2].i);
    pOutput[(3 * 4 + 2) * nOutputStride].r = NAVG(anStage2[1 * 4 + 2].r, anStage2[3 * 4 + 2].i);
    pOutput[(3 * 4 + 2) * nOutputStride].i = AVG_ROUND(anStage2[1 * 4 + 2].i, anStage2[3 * 4 + 2].r);

    anTmp[0 * 4 + 3].r = anStage1[0 * 4 + 3].r;
    anTmp[0 * 4 + 3].i = anStage1[0 * 4 + 3].i;
    anTmp[1 * 4 + 3].r = MPY(SIN_PI_8, anStage1[1 * 4 + 3].r) + MPY(COS_PI_8, anStage1[1 * 4 + 3].i);
    anTmp[1 * 4 + 3].i = -MPY(COS_PI_8, anStage1[1 * 4 + 3].r) + MPY(SIN_PI_8, anStage1[1 * 4 + 3].i);
    anTmp[2 * 4 + 3].r = -MPY(COS_PI_4, anStage1[2 * 4 + 3].r) + MPY(COS_PI_4, anStage1[2 * 4 + 3].i);
    anTmp[2 * 4 + 3].i = MPY(COS_PI_4, anStage1[2 * 4 + 3].r) + MPY(COS_PI_4, anStage1[2 * 4 + 3].i);
    anTmp[3 * 4 + 3].r = MPY(COS_PI_8, anStage1[3 * 4 + 3].r) + MPY(SIN_PI_8, anStage1[3 * 4 + 3].i);
    anTmp[3 * 4 + 3].i = MPY(SIN_PI_8, anStage1[3 * 4 + 3].r) - MPY(COS_PI_8, anStage1[3 * 4 + 3].i);
    // input: 16-bit signed integer
    // output: 16-bit signed integer
    anStage2[0 * 4 + 3].r = AVG_ROUND(anTmp[0 * 4 + 3].r, anTmp[2 * 4 + 3].r);
    anStage2[0 * 4 + 3].i = NAVG(anTmp[0 * 4 + 3].i, anTmp[2 * 4 + 3].i);
    anStage2[1 * 4 + 3].r = NAVG(anTmp[0 * 4 + 3].r, anTmp[2 * 4 + 3].r);
    anStage2[1 * 4 + 3].i = AVG_ROUND(anTmp[0 * 4 + 3].i, anTmp[2 * 4 + 3].i);
    anStage2[2 * 4 + 3].r = NAVG(anTmp[1 * 4 + 3].r, anTmp[3 * 4 + 3].r);
    anStage2[2 * 4 + 3].i = AVG_ROUND(anTmp[1 * 4 + 3].i, anTmp[3 * 4 + 3].i);
    anStage2[3 * 4 + 3].r = AVG_ROUND(anTmp[1 * 4 + 3].r, anTmp[3 * 4 + 3].r);
    anStage2[3 * 4 + 3].i = NAVG(anTmp[1 * 4 + 3].i, anTmp[3 * 4 + 3].i);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    pOutput[(0 * 4 + 3) * nOutputStride].r = AVG_ROUND(anStage2[0 * 4 + 3].r, anStage2[2 * 4 + 3].r);
    pOutput[(0 * 4 + 3) * nOutputStride].i = AVG_ROUND(anStage2[0 * 4 + 3].i, anStage2[2 * 4 + 3].i);
    pOutput[(1 * 4 + 3) * nOutputStride].r = AVG_ROUND(anStage2[1 * 4 + 3].r, anStage2[3 * 4 + 3].i);
    pOutput[(1 * 4 + 3) * nOutputStride].i = NAVG(anStage2[1 * 4 + 3].i, anStage2[3 * 4 + 3].r);
    pOutput[(2 * 4 + 3) * nOutputStride].r = NAVG(anStage2[0 * 4 + 3].r, anStage2[2 * 4 + 3].r);
    pOutput[(2 * 4 + 3) * nOutputStride].i = NAVG(anStage2[0 * 4 + 3].i, anStage2[2 * 4 + 3].i);
    pOutput[(3 * 4 + 3) * nOutputStride].r = NAVG(anStage2[1 * 4 + 3].r, anStage2[3 * 4 + 3].i);
    pOutput[(3 * 4 + 3) * nOutputStride].i = AVG_ROUND(anStage2[1 * 4 + 3].i, anStage2[3 * 4 + 3].r);
}

void RFFT16x16_2_2(complex_t *pOutput, const uint16_t nOutputStride,
                   const real_t *pInput, const uint16_t nInputStride)
{
    complex_t anTmp[16 * RFFT_LENGTH];

    for (int32_t i = 0; i < 16; i++)
    {
        RFFT1x16(&anTmp[i * RFFT_LENGTH], &pInput[i * 2 * nInputStride], 2);
    }

    for (int32_t i = 0; i < RFFT_LENGTH; i++)
    {
        CFFT16x1(&pOutput[i * 2], 2 * nOutputStride, &anTmp[i]);
    }
}
void RFFT16x16_4_1(complex_t *pOutput, const uint16_t nOutputStride,
                   const real_t *pInput, const uint16_t nInputStride)
{
    complex_t anTmp[16 * RFFT_LENGTH];
    /*for(int32_t j=0; j<16;j++)
        {
    for(int32_t i=0; i<16; i++)
        {

            printf("RFFT1x4 out r%d%d = %d \n",i,j, pInput[i*nInputStride+j]);

        }
        }*/
    for (int32_t i = 0; i < 16; i++)
    {
        RFFT1x16(&anTmp[i * RFFT_LENGTH], &pInput[i * nInputStride], 1);
    }

    for (int32_t i = 0; i < RFFT_LENGTH; i++)
    {
        CFFT16x1(&pOutput[i], nOutputStride, &anTmp[i]);
    }
}
void RFFT16x16_4(complex_t *pOutput, const uint16_t nOutputStride,
                 const real_t *pInput, const uint16_t nInputStride)
{

    RFFT16x16_4_1(pOutput, nOutputStride, pInput, nInputStride);
    RFFT16x16_4_1(pOutput + 16, nOutputStride, pInput + 16, nInputStride);
    RFFT16x16_4_1(pOutput + 32, nOutputStride, pInput + 32, nInputStride);
    RFFT16x16_4_1(pOutput + 48, nOutputStride, pInput + 48, nInputStride);
}

static void CIFFT4x1(complex_t *pOutput, const complex_t *pInput, const uint16_t nInputStride)
{
    complex_t anStage1[4];

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    anStage1[0].r = AVG_ROUND(pInput[0 * 4 * nInputStride].r, pInput[2 * 4 * nInputStride].r);
    anStage1[0].i = AVG_ROUND(pInput[0 * 4 * nInputStride].i, pInput[2 * 4 * nInputStride].i);
    anStage1[1].r = NAVG(pInput[0 * 4 * nInputStride].r, pInput[2 * 4 * nInputStride].r);
    anStage1[1].i = NAVG(pInput[0 * 4 * nInputStride].i, pInput[2 * 4 * nInputStride].i);
    anStage1[2].r = AVG_ROUND(pInput[1 * 4 * nInputStride].r, pInput[3 * 4 * nInputStride].r);
    anStage1[2].i = AVG_ROUND(pInput[1 * 4 * nInputStride].i, pInput[3 * 4 * nInputStride].i);
    anStage1[3].r = NAVG(pInput[1 * 4 * nInputStride].r, pInput[3 * 4 * nInputStride].r);
    anStage1[3].i = NAVG(pInput[1 * 4 * nInputStride].i, pInput[3 * 4 * nInputStride].i);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    pOutput[0].r = AVG_ROUND(anStage1[0].r, anStage1[2].r);
    pOutput[0].i = AVG_ROUND(anStage1[0].i, anStage1[2].i);
    pOutput[1].r = NAVG(anStage1[1].r, anStage1[3].i);
    pOutput[1].i = AVG_ROUND(anStage1[1].i, anStage1[3].r);
    pOutput[2].r = NAVG(anStage1[0].r, anStage1[2].r);
    pOutput[2].i = NAVG(anStage1[0].i, anStage1[2].i);
    pOutput[3].r = AVG_ROUND(anStage1[1].r, anStage1[3].i);
    pOutput[3].i = NAVG(anStage1[1].i, anStage1[3].r);
}

static void CIFFT16x1(complex_t *pOutput, const complex_t *pInput, const uint16_t nInputStride)
{
    complex_t anStage1[16];
    complex_t anTmp[4 * 4];
    complex_t anStage2[4 * 4];

    for (int32_t i = 0; i < 4; i++)
    {
        // input: 16-bit signed integer
        // output: 16-bit signed integer
        CIFFT4x1(&anStage1[i * 4], &pInput[i * nInputStride], nInputStride);
    }

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    anStage2[0 * 4 + 0].r = AVG_ROUND(anStage1[0 * 4 + 0].r, anStage1[2 * 4 + 0].r);
    anStage2[0 * 4 + 0].i = AVG_ROUND(anStage1[0 * 4 + 0].i, anStage1[2 * 4 + 0].i); // C
    anStage2[1 * 4 + 0].r = NAVG(anStage1[0 * 4 + 0].r, anStage1[2 * 4 + 0].r);
    anStage2[1 * 4 + 0].i = NAVG(anStage1[0 * 4 + 0].i, anStage1[2 * 4 + 0].i);
    anStage2[2 * 4 + 0].r = AVG_ROUND(anStage1[1 * 4 + 0].r, anStage1[3 * 4 + 0].r);
    anStage2[2 * 4 + 0].i = AVG_ROUND(anStage1[1 * 4 + 0].i, anStage1[3 * 4 + 0].i);
    anStage2[3 * 4 + 0].r = NAVG(anStage1[1 * 4 + 0].r, anStage1[3 * 4 + 0].r);
    anStage2[3 * 4 + 0].i = NAVG(anStage1[1 * 4 + 0].i, anStage1[3 * 4 + 0].i);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    pOutput[(0 * 4 + 0) * RFFT_LENGTH].r = AVG_ROUND(anStage2[0 * 4 + 0].r, anStage2[2 * 4 + 0].r);
    pOutput[(0 * 4 + 0) * RFFT_LENGTH].i = AVG_ROUND(anStage2[0 * 4 + 0].i, anStage2[2 * 4 + 0].i);
    pOutput[(1 * 4 + 0) * RFFT_LENGTH].r = NAVG(anStage2[1 * 4 + 0].r, anStage2[3 * 4 + 0].i);
    pOutput[(1 * 4 + 0) * RFFT_LENGTH].i = AVG_ROUND(anStage2[1 * 4 + 0].i, anStage2[3 * 4 + 0].r);
    pOutput[(2 * 4 + 0) * RFFT_LENGTH].r = NAVG(anStage2[0 * 4 + 0].r, anStage2[2 * 4 + 0].r);
    pOutput[(2 * 4 + 0) * RFFT_LENGTH].i = NAVG(anStage2[0 * 4 + 0].i, anStage2[2 * 4 + 0].i);
    pOutput[(3 * 4 + 0) * RFFT_LENGTH].r = AVG_ROUND(anStage2[1 * 4 + 0].r, anStage2[3 * 4 + 0].i);
    pOutput[(3 * 4 + 0) * RFFT_LENGTH].i = NAVG(anStage2[1 * 4 + 0].i, anStage2[3 * 4 + 0].r);

    anTmp[0 * 4 + 1].r = anStage1[0 * 4 + 1].r;
    anTmp[0 * 4 + 1].i = anStage1[0 * 4 + 1].i;
    anTmp[1 * 4 + 1].r = MPY(COS_PI_8, anStage1[1 * 4 + 1].r) - MPY(SIN_PI_8, anStage1[1 * 4 + 1].i);
    anTmp[1 * 4 + 1].i = MPY(SIN_PI_8, anStage1[1 * 4 + 1].r) + MPY(COS_PI_8, anStage1[1 * 4 + 1].i);
    anTmp[2 * 4 + 1].r = MPY(COS_PI_4, anStage1[2 * 4 + 1].r) - MPY(COS_PI_4, anStage1[2 * 4 + 1].i);
    anTmp[2 * 4 + 1].i = MPY(COS_PI_4, anStage1[2 * 4 + 1].r) + MPY(COS_PI_4, anStage1[2 * 4 + 1].i);
    anTmp[3 * 4 + 1].r = MPY(SIN_PI_8, anStage1[3 * 4 + 1].r) - MPY(COS_PI_8, anStage1[3 * 4 + 1].i);
    anTmp[3 * 4 + 1].i = MPY(COS_PI_8, anStage1[3 * 4 + 1].r) + MPY(SIN_PI_8, anStage1[3 * 4 + 1].i);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    anStage2[0 * 4 + 1].r = AVG_ROUND(anTmp[0 * 4 + 1].r, anTmp[2 * 4 + 1].r);
    anStage2[0 * 4 + 1].i = AVG_ROUND(anTmp[0 * 4 + 1].i, anTmp[2 * 4 + 1].i);
    anStage2[1 * 4 + 1].r = NAVG(anTmp[0 * 4 + 1].r, anTmp[2 * 4 + 1].r);
    anStage2[1 * 4 + 1].i = NAVG(anTmp[0 * 4 + 1].i, anTmp[2 * 4 + 1].i);
    anStage2[2 * 4 + 1].r = AVG_ROUND(anTmp[1 * 4 + 1].r, anTmp[3 * 4 + 1].r);
    anStage2[2 * 4 + 1].i = AVG_ROUND(anTmp[1 * 4 + 1].i, anTmp[3 * 4 + 1].i);
    anStage2[3 * 4 + 1].r = NAVG(anTmp[1 * 4 + 1].r, anTmp[3 * 4 + 1].r);
    anStage2[3 * 4 + 1].i = NAVG(anTmp[1 * 4 + 1].i, anTmp[3 * 4 + 1].i);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    pOutput[(0 * 4 + 1) * RFFT_LENGTH].r = AVG_ROUND(anStage2[0 * 4 + 1].r, anStage2[2 * 4 + 1].r);
    pOutput[(0 * 4 + 1) * RFFT_LENGTH].i = AVG_ROUND(anStage2[0 * 4 + 1].i, anStage2[2 * 4 + 1].i);
    pOutput[(1 * 4 + 1) * RFFT_LENGTH].r = NAVG(anStage2[1 * 4 + 1].r, anStage2[3 * 4 + 1].i);
    pOutput[(1 * 4 + 1) * RFFT_LENGTH].i = AVG_ROUND(anStage2[1 * 4 + 1].i, anStage2[3 * 4 + 1].r);
    pOutput[(2 * 4 + 1) * RFFT_LENGTH].r = NAVG(anStage2[0 * 4 + 1].r, anStage2[2 * 4 + 1].r);
    pOutput[(2 * 4 + 1) * RFFT_LENGTH].i = NAVG(anStage2[0 * 4 + 1].i, anStage2[2 * 4 + 1].i);
    pOutput[(3 * 4 + 1) * RFFT_LENGTH].r = AVG_ROUND(anStage2[1 * 4 + 1].r, anStage2[3 * 4 + 1].i);
    pOutput[(3 * 4 + 1) * RFFT_LENGTH].i = NAVG(anStage2[1 * 4 + 1].i, anStage2[3 * 4 + 1].r);

    anTmp[0 * 4 + 2].r = anStage1[0 * 4 + 2].r;
    anTmp[0 * 4 + 2].i = anStage1[0 * 4 + 2].i;
    anTmp[1 * 4 + 2].r = MPY(COS_PI_4, anStage1[1 * 4 + 2].r) - MPY(COS_PI_4, anStage1[1 * 4 + 2].i);
    anTmp[1 * 4 + 2].i = MPY(COS_PI_4, anStage1[1 * 4 + 2].r) + MPY(COS_PI_4, anStage1[1 * 4 + 2].i);
    anTmp[2 * 4 + 2].r = anStage1[2 * 4 + 2].i;
    anTmp[2 * 4 + 2].i = anStage1[2 * 4 + 2].r;
    anTmp[3 * 4 + 2].r = MPY(COS_PI_4, anStage1[3 * 4 + 2].r) + MPY(COS_PI_4, anStage1[3 * 4 + 2].i);
    anTmp[3 * 4 + 2].i = MPY(COS_PI_4, anStage1[3 * 4 + 2].r) - MPY(COS_PI_4, anStage1[3 * 4 + 2].i);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    anStage2[0 * 4 + 2].r = NAVG(anTmp[0 * 4 + 2].r, anTmp[2 * 4 + 2].r);
    anStage2[0 * 4 + 2].i = AVG_ROUND(anTmp[0 * 4 + 2].i, anTmp[2 * 4 + 2].i);
    anStage2[1 * 4 + 2].r = AVG_ROUND(anTmp[0 * 4 + 2].r, anTmp[2 * 4 + 2].r);
    anStage2[1 * 4 + 2].i = NAVG(anTmp[0 * 4 + 2].i, anTmp[2 * 4 + 2].i);
    anStage2[2 * 4 + 2].r = NAVG(anTmp[1 * 4 + 2].r, anTmp[3 * 4 + 2].r);
    anStage2[2 * 4 + 2].i = AVG_ROUND(anTmp[1 * 4 + 2].i, anTmp[3 * 4 + 2].i);
    anStage2[3 * 4 + 2].r = AVG_ROUND(anTmp[1 * 4 + 2].r, anTmp[3 * 4 + 2].r);
    anStage2[3 * 4 + 2].i = NAVG(anTmp[1 * 4 + 2].i, anTmp[3 * 4 + 2].i);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    pOutput[(0 * 4 + 2) * RFFT_LENGTH].r = AVG_ROUND(anStage2[0 * 4 + 2].r, anStage2[2 * 4 + 2].r);
    pOutput[(0 * 4 + 2) * RFFT_LENGTH].i = AVG_ROUND(anStage2[0 * 4 + 2].i, anStage2[2 * 4 + 2].i);
    pOutput[(1 * 4 + 2) * RFFT_LENGTH].r = NAVG(anStage2[1 * 4 + 2].r, anStage2[3 * 4 + 2].i);
    pOutput[(1 * 4 + 2) * RFFT_LENGTH].i = AVG_ROUND(anStage2[1 * 4 + 2].i, anStage2[3 * 4 + 2].r);
    pOutput[(2 * 4 + 2) * RFFT_LENGTH].r = NAVG(anStage2[0 * 4 + 2].r, anStage2[2 * 4 + 2].r);
    pOutput[(2 * 4 + 2) * RFFT_LENGTH].i = NAVG(anStage2[0 * 4 + 2].i, anStage2[2 * 4 + 2].i);
    pOutput[(3 * 4 + 2) * RFFT_LENGTH].r = AVG_ROUND(anStage2[1 * 4 + 2].r, anStage2[3 * 4 + 2].i);
    pOutput[(3 * 4 + 2) * RFFT_LENGTH].i = NAVG(anStage2[1 * 4 + 2].i, anStage2[3 * 4 + 2].r);

    anTmp[0 * 4 + 3].r = anStage1[0 * 4 + 3].r;
    anTmp[0 * 4 + 3].i = anStage1[0 * 4 + 3].i;
    anTmp[1 * 4 + 3].r = MPY(SIN_PI_8, anStage1[1 * 4 + 3].r) - MPY(COS_PI_8, anStage1[1 * 4 + 3].i);
    anTmp[1 * 4 + 3].i = MPY(COS_PI_8, anStage1[1 * 4 + 3].r) + MPY(SIN_PI_8, anStage1[1 * 4 + 3].i);
    anTmp[2 * 4 + 3].r = MPY(COS_PI_4, anStage1[2 * 4 + 3].r) + MPY(COS_PI_4, anStage1[2 * 4 + 3].i);
    anTmp[2 * 4 + 3].i = MPY(COS_PI_4, anStage1[2 * 4 + 3].r) - MPY(COS_PI_4, anStage1[2 * 4 + 3].i);
    anTmp[3 * 4 + 3].r = -MPY(COS_PI_8, anStage1[3 * 4 + 3].r) + MPY(SIN_PI_8, anStage1[3 * 4 + 3].i);

    anTmp[3 * 4 + 3].i = MPY(SIN_PI_8, anStage1[3 * 4 + 3].r) + MPY(COS_PI_8, anStage1[3 * 4 + 3].i);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    anStage2[0 * 4 + 3].r = NAVG(anTmp[0 * 4 + 3].r, anTmp[2 * 4 + 3].r);
    anStage2[0 * 4 + 3].i = AVG_ROUND(anTmp[0 * 4 + 3].i, anTmp[2 * 4 + 3].i);
    anStage2[1 * 4 + 3].r = AVG_ROUND(anTmp[0 * 4 + 3].r, anTmp[2 * 4 + 3].r);
    anStage2[1 * 4 + 3].i = NAVG(anTmp[0 * 4 + 3].i, anTmp[2 * 4 + 3].i);
    anStage2[2 * 4 + 3].r = AVG_ROUND(anTmp[1 * 4 + 3].r, anTmp[3 * 4 + 3].r);
    anStage2[2 * 4 + 3].i = NAVG(anTmp[1 * 4 + 3].i, anTmp[3 * 4 + 3].i);
    anStage2[3 * 4 + 3].r = NAVG(anTmp[1 * 4 + 3].r, anTmp[3 * 4 + 3].r);

    anStage2[3 * 4 + 3].i = AVG_ROUND(anTmp[1 * 4 + 3].i, anTmp[3 * 4 + 3].i);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    pOutput[(0 * 4 + 3) * RFFT_LENGTH].r = AVG_ROUND(anStage2[0 * 4 + 3].r, anStage2[2 * 4 + 3].r);
    pOutput[(0 * 4 + 3) * RFFT_LENGTH].i = AVG_ROUND(anStage2[0 * 4 + 3].i, anStage2[2 * 4 + 3].i);

    pOutput[(1 * 4 + 3) * RFFT_LENGTH].r = NAVG(anStage2[1 * 4 + 3].r, anStage2[3 * 4 + 3].i);
    pOutput[(1 * 4 + 3) * RFFT_LENGTH].i = AVG_ROUND(anStage2[1 * 4 + 3].i, anStage2[3 * 4 + 3].r);
    pOutput[(2 * 4 + 3) * RFFT_LENGTH].r = NAVG(anStage2[0 * 4 + 3].r, anStage2[2 * 4 + 3].r);
    pOutput[(2 * 4 + 3) * RFFT_LENGTH].i = NAVG(anStage2[0 * 4 + 3].i, anStage2[2 * 4 + 3].i);
    pOutput[(3 * 4 + 3) * RFFT_LENGTH].r = AVG_ROUND(anStage2[1 * 4 + 3].r, anStage2[3 * 4 + 3].i);
    pOutput[(3 * 4 + 3) * RFFT_LENGTH].i = NAVG(anStage2[1 * 4 + 3].i, anStage2[3 * 4 + 3].r);
}

static void RIFFT1x16(real_t *pOutput, const uint16_t nOffset, const complex_t *pInput)
{
    complex_t anTmp1[4 * 4];
    complex_t anStage1[4 * 4];
    complex_t anTmp2[4 * 4];
    real_t anStage2[4 * 4];

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    anTmp1[0 * 4 + 0].r = AVG_ROUND(pInput[0 * 4 + 0].r, pInput[2 * 4 + 0].r);

    anTmp1[0 * 4 + 1].r = NAVG(pInput[0 * 4 + 0].r, pInput[2 * 4 + 0].r);
    anTmp1[0 * 4 + 2].r = AVG_ROUND(pInput[1 * 4 + 0].r, pInput[1 * 4 + 0].r);
    anTmp1[0 * 4 + 3].i = AVG_ROUND(pInput[1 * 4 + 0].i, pInput[1 * 4 + 0].i);

    // input: 16-bit signed integer
    // output: 17-bit signed integer
    anStage1[0 * 4 + 0].r = SAT(anTmp1[0 * 4 + 0].r + anTmp1[0 * 4 + 2].r);

    anStage1[0 * 4 + 1].r = SAT(anTmp1[0 * 4 + 1].r - anTmp1[0 * 4 + 3].i);
    anStage1[0 * 4 + 2].r = SAT(anTmp1[0 * 4 + 0].r - anTmp1[0 * 4 + 2].r);

    anStage1[0 * 4 + 3].r = SAT(anTmp1[0 * 4 + 1].r + anTmp1[0 * 4 + 3].i);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    anTmp1[1 * 4 + 0].r = AVG_ROUND(pInput[0 * 4 + 1].r, pInput[1 * 4 + 3].r);
    anTmp1[1 * 4 + 0].i = NAVG(pInput[0 * 4 + 1].i, pInput[1 * 4 + 3].i);
    anTmp1[1 * 4 + 1].r = NAVG(pInput[0 * 4 + 1].r, pInput[1 * 4 + 3].r);
    anTmp1[1 * 4 + 1].i = AVG_ROUND(pInput[0 * 4 + 1].i, pInput[1 * 4 + 3].i);
    anTmp1[1 * 4 + 2].r = AVG_ROUND(pInput[1 * 4 + 1].r, pInput[0 * 4 + 3].r);
    anTmp1[1 * 4 + 2].i = NAVG(pInput[1 * 4 + 1].i, pInput[0 * 4 + 3].i);
    anTmp1[1 * 4 + 3].r = NAVG(pInput[1 * 4 + 1].r, pInput[0 * 4 + 3].r);
    anTmp1[1 * 4 + 3].i = AVG_ROUND(pInput[1 * 4 + 1].i, pInput[0 * 4 + 3].i);

    // input: 16-bit signed integer
    // output: 17-bit signed integer

    //	printf("RFFT1x4 out r = %d \n", anTmp1[1*4+2].r);
    anStage1[1 * 4 + 0].r = SAT(anTmp1[1 * 4 + 0].r + anTmp1[1 * 4 + 2].r);
    anStage1[1 * 4 + 0].i = SAT(anTmp1[1 * 4 + 0].i + anTmp1[1 * 4 + 2].i);
    anStage1[1 * 4 + 1].r = SAT(anTmp1[1 * 4 + 1].r - anTmp1[1 * 4 + 3].i);
    anStage1[1 * 4 + 1].i = SAT(anTmp1[1 * 4 + 1].i + anTmp1[1 * 4 + 3].r);
    anStage1[1 * 4 + 2].r = SAT(anTmp1[1 * 4 + 0].r - anTmp1[1 * 4 + 2].r);
    anStage1[1 * 4 + 2].i = SAT(anTmp1[1 * 4 + 0].i - anTmp1[1 * 4 + 2].i);
    anStage1[1 * 4 + 3].r = SAT(anTmp1[1 * 4 + 1].r + anTmp1[1 * 4 + 3].i);
    anStage1[1 * 4 + 3].i = SAT(anTmp1[1 * 4 + 1].i - anTmp1[1 * 4 + 3].r);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    anTmp1[2 * 4 + 0].r = AVG_ROUND(pInput[0 * 4 + 2].r, pInput[1 * 4 + 2].r);
    anTmp1[2 * 4 + 0].i = NAVG(pInput[0 * 4 + 2].i, pInput[1 * 4 + 2].i);
    anTmp1[2 * 4 + 1].r = NAVG(pInput[0 * 4 + 2].r, pInput[1 * 4 + 2].r);
    anTmp1[2 * 4 + 1].i = AVG_ROUND(pInput[0 * 4 + 2].i, pInput[1 * 4 + 2].i);
    anTmp1[2 * 4 + 2].r = AVG_ROUND(pInput[1 * 4 + 2].r, pInput[0 * 4 + 2].r);
    anTmp1[2 * 4 + 2].i = NAVG(pInput[1 * 4 + 2].i, pInput[0 * 4 + 2].i);
    anTmp1[2 * 4 + 3].r = NAVG(pInput[1 * 4 + 2].r, pInput[0 * 4 + 2].r);
    anTmp1[2 * 4 + 3].i = AVG_ROUND(pInput[1 * 4 + 2].i, pInput[0 * 4 + 2].i);

    // input: 16-bit signed integer
    // output: 17-bit signed integer
    anStage1[2 * 4 + 0].r = SAT(anTmp1[2 * 4 + 0].r + anTmp1[2 * 4 + 2].r);
    anStage1[2 * 4 + 1].r = SAT(anTmp1[2 * 4 + 1].r - anTmp1[2 * 4 + 3].i);
    anStage1[2 * 4 + 1].i = SAT(anTmp1[2 * 4 + 1].i + anTmp1[2 * 4 + 3].r);
    anStage1[2 * 4 + 2].i = SAT(anTmp1[2 * 4 + 0].i - anTmp1[2 * 4 + 2].i);
    anStage1[2 * 4 + 3].r = SAT(anTmp1[2 * 4 + 1].r + anTmp1[2 * 4 + 3].i);
    anStage1[2 * 4 + 3].i = SAT(anTmp1[2 * 4 + 1].i - anTmp1[2 * 4 + 3].r);

    // input: 16-bit signed integer
    // output: 16-bit signed integer
    anTmp1[3 * 4 + 0].r = AVG_ROUND(pInput[0 * 4 + 3].r, pInput[1 * 4 + 1].r);
    anTmp1[3 * 4 + 0].i = NAVG(pInput[0 * 4 + 3].i, pInput[1 * 4 + 1].i);
    anTmp1[3 * 4 + 1].r = NAVG(pInput[0 * 4 + 3].r, pInput[1 * 4 + 1].r);
    anTmp1[3 * 4 + 1].i = AVG_ROUND(pInput[0 * 4 + 3].i, pInput[1 * 4 + 1].i);
    anTmp1[3 * 4 + 2].r = AVG_ROUND(pInput[1 * 4 + 3].r, pInput[0 * 4 + 1].r);
    anTmp1[3 * 4 + 2].i = NAVG(pInput[1 * 4 + 3].i, pInput[0 * 4 + 1].i);
    anTmp1[3 * 4 + 3].r = NAVG(pInput[1 * 4 + 3].r, pInput[0 * 4 + 1].r);
    anTmp1[3 * 4 + 3].i = AVG_ROUND(pInput[1 * 4 + 3].i, pInput[0 * 4 + 1].i);

    // input: 16-bit signed integer
    // output: 17-bit signed integer
    anStage1[3 * 4 + 0].r = SAT(anTmp1[3 * 4 + 0].r + anTmp1[3 * 4 + 2].r);
    anStage1[3 * 4 + 0].i = SAT(anTmp1[3 * 4 + 0].i + anTmp1[3 * 4 + 2].i);
    anStage1[3 * 4 + 1].r = SAT(anTmp1[3 * 4 + 1].r - anTmp1[3 * 4 + 3].i);
    anStage1[3 * 4 + 1].i = SAT(anTmp1[3 * 4 + 1].i + anTmp1[3 * 4 + 3].r);
    anStage1[3 * 4 + 2].r = SAT(anTmp1[3 * 4 + 0].r - anTmp1[3 * 4 + 2].r);
    anStage1[3 * 4 + 2].i = SAT(anTmp1[3 * 4 + 0].i - anTmp1[3 * 4 + 2].i);
    anStage1[3 * 4 + 3].r = SAT(anTmp1[3 * 4 + 1].r + anTmp1[3 * 4 + 3].i);
    anStage1[3 * 4 + 3].i = SAT(anTmp1[3 * 4 + 1].i - anTmp1[3 * 4 + 3].r);

    // input: 17-bit signed integer
    // output: 18-bit signed integer

    anStage2[0 * 4 + 0] = SAT(anStage1[0 * 4 + 0].r + anStage1[2 * 4 + 0].r);
    anStage2[1 * 4 + 0] = SAT(anStage1[0 * 4 + 0].r - anStage1[2 * 4 + 0].r);

    anStage2[2 * 4 + 0] = SAT(anStage1[1 * 4 + 0].r + anStage1[3 * 4 + 0].r);
    anStage2[3 * 4 + 0] = SAT(anStage1[1 * 4 + 0].i - anStage1[3 * 4 + 0].i);

    // input: 18-bit signed integer
    // output: 19-bit signed integer

    pOutput[(0 * 4 + 0) * nOffset] = SAT(anStage2[0 * 4 + 0] + anStage2[2 * 4 + 0]);

    pOutput[(1 * 4 + 0) * nOffset] = SAT(anStage2[1 * 4 + 0] - anStage2[3 * 4 + 0]);

    pOutput[(2 * 4 + 0) * nOffset] = SAT(anStage2[0 * 4 + 0] - anStage2[2 * 4 + 0]);
    pOutput[(3 * 4 + 0) * nOffset] = SAT(anStage2[1 * 4 + 0] + anStage2[3 * 4 + 0]);

    anTmp2[0 * 4 + 1].r = anStage1[0 * 4 + 1].r;
    anTmp2[1 * 4 + 1].r = MPY(COS_PI_8, anStage1[1 * 4 + 1].r) - MPY(SIN_PI_8, anStage1[1 * 4 + 1].i);
    anTmp2[1 * 4 + 1].i = MPY(SIN_PI_8, anStage1[1 * 4 + 1].r) + MPY(COS_PI_8, anStage1[1 * 4 + 1].i);
    anTmp2[2 * 4 + 1].r = MPY(COS_PI_4, anStage1[2 * 4 + 1].r) - MPY(COS_PI_4, anStage1[2 * 4 + 1].i);
    anTmp2[3 * 4 + 1].r = MPY(SIN_PI_8, anStage1[3 * 4 + 1].r) - MPY(COS_PI_8, anStage1[3 * 4 + 1].i);

    anTmp2[3 * 4 + 1].i = MPY(COS_PI_8, anStage1[3 * 4 + 1].r) + MPY(SIN_PI_8, anStage1[3 * 4 + 1].i);

    // input: 17-bit signed integer
    // output: 18-bit signed integer
    anStage2[0 * 4 + 1] = SAT(anTmp2[0 * 4 + 1].r + anTmp2[2 * 4 + 1].r);
    anStage2[1 * 4 + 1] = SAT(anTmp2[0 * 4 + 1].r - anTmp2[2 * 4 + 1].r);
    anStage2[2 * 4 + 1] = SAT(anTmp2[1 * 4 + 1].r + anTmp2[3 * 4 + 1].r);

    anStage2[3 * 4 + 1] = SAT(anTmp2[1 * 4 + 1].i - anTmp2[3 * 4 + 1].i);

    // input: 18-bit signed integer
    // output: 19-bit signed integer

    pOutput[(0 * 4 + 1) * nOffset] = SAT(anStage2[0 * 4 + 1] + anStage2[2 * 4 + 1]);
    pOutput[(1 * 4 + 1) * nOffset] = SAT(anStage2[1 * 4 + 1] - anStage2[3 * 4 + 1]);
    pOutput[(2 * 4 + 1) * nOffset] = SAT(anStage2[0 * 4 + 1] - anStage2[2 * 4 + 1]);
    pOutput[(3 * 4 + 1) * nOffset] = SAT(anStage2[1 * 4 + 1] + anStage2[3 * 4 + 1]);

    anTmp2[0 * 4 + 2].r = anStage1[0 * 4 + 2].r;
    anTmp2[1 * 4 + 2].r = MPY(COS_PI_4, anStage1[1 * 4 + 2].r) - MPY(COS_PI_4, anStage1[1 * 4 + 2].i);
    anTmp2[1 * 4 + 2].i = MPY(COS_PI_4, anStage1[1 * 4 + 2].r) + MPY(COS_PI_4, anStage1[1 * 4 + 2].i);
    anTmp2[2 * 4 + 2].r = anStage1[2 * 4 + 2].i;
    anTmp2[3 * 4 + 2].r = MPY(COS_PI_4, anStage1[3 * 4 + 2].r) + MPY(COS_PI_4, anStage1[3 * 4 + 2].i);
    anTmp2[3 * 4 + 2].i = MPY(COS_PI_4, anStage1[3 * 4 + 2].r) - MPY(COS_PI_4, anStage1[3 * 4 + 2].i);

    // input: 17-bit signed integer
    // output: 18-bit signed integer
    anStage2[0 * 4 + 2] = SAT(anTmp2[0 * 4 + 2].r - anTmp2[2 * 4 + 2].r);
    anStage2[1 * 4 + 2] = SAT(anTmp2[0 * 4 + 2].r + anTmp2[2 * 4 + 2].r);
    anStage2[2 * 4 + 2] = SAT(anTmp2[1 * 4 + 2].r - anTmp2[3 * 4 + 2].r);
    anStage2[3 * 4 + 2] = SAT(anTmp2[1 * 4 + 2].i - anTmp2[3 * 4 + 2].i);

    // input: 18-bit signed integer
    // output: 19-bit signed integer
    pOutput[(0 * 4 + 2) * nOffset] = SAT(anStage2[0 * 4 + 2] + anStage2[2 * 4 + 2]);
    pOutput[(1 * 4 + 2) * nOffset] = SAT(anStage2[1 * 4 + 2] - anStage2[3 * 4 + 2]);
    pOutput[(2 * 4 + 2) * nOffset] = SAT(anStage2[0 * 4 + 2] - anStage2[2 * 4 + 2]);
    pOutput[(3 * 4 + 2) * nOffset] = SAT(anStage2[1 * 4 + 2] + anStage2[3 * 4 + 2]);

    anTmp2[0 * 4 + 3].r = anStage1[0 * 4 + 3].r;
    anTmp2[1 * 4 + 3].r = MPY(SIN_PI_8, anStage1[1 * 4 + 3].r) - MPY(COS_PI_8, anStage1[1 * 4 + 3].i);
    anTmp2[1 * 4 + 3].i = MPY(COS_PI_8, anStage1[1 * 4 + 3].r) + MPY(SIN_PI_8, anStage1[1 * 4 + 3].i);
    anTmp2[2 * 4 + 3].r = MPY(COS_PI_4, anStage1[2 * 4 + 3].r) + MPY(COS_PI_4, anStage1[2 * 4 + 3].i);
    anTmp2[3 * 4 + 3].r = -MPY(COS_PI_8, anStage1[3 * 4 + 3].r) + MPY(SIN_PI_8, anStage1[3 * 4 + 3].i);
    anTmp2[3 * 4 + 3].i = MPY(SIN_PI_8, anStage1[3 * 4 + 3].r) + MPY(COS_PI_8, anStage1[3 * 4 + 3].i);

    // input: 17-bit signed integer
    // output: 18-bit signed integer

    anStage2[0 * 4 + 3] = SAT(anTmp2[0 * 4 + 3].r - anTmp2[2 * 4 + 3].r);
    anStage2[1 * 4 + 3] = SAT(anTmp2[0 * 4 + 3].r + anTmp2[2 * 4 + 3].r);
    anStage2[2 * 4 + 3] = SAT(anTmp2[1 * 4 + 3].r + anTmp2[3 * 4 + 3].r);
    anStage2[3 * 4 + 3] = SAT(anTmp2[1 * 4 + 3].i + anTmp2[3 * 4 + 3].i);

    // input: 18-bit signed integer
    // output: 19-bit signed integer

    pOutput[(0 * 4 + 3) * nOffset] = SAT(anStage2[0 * 4 + 3] + anStage2[2 * 4 + 3]);
    pOutput[(1 * 4 + 3) * nOffset] = SAT(anStage2[1 * 4 + 3] - anStage2[3 * 4 + 3]);
    pOutput[(2 * 4 + 3) * nOffset] = SAT(anStage2[0 * 4 + 3] - anStage2[2 * 4 + 3]);
    pOutput[(3 * 4 + 3) * nOffset] = SAT(anStage2[1 * 4 + 3] + anStage2[3 * 4 + 3]);
}

void RIFFT16x16_2_2(real_t *pOutput, const uint16_t nOutputStride,
                    const complex_t *pInput, const uint16_t nInputStride)
{
    complex_t anTmp[16 * RFFT_LENGTH];

    for (int32_t i = 0; i < RFFT_LENGTH; i++)
    {
        CIFFT16x1(&anTmp[i], &pInput[i * 2], 2 * nInputStride);
    }

    for (int32_t i = 0; i < 16; i++)
    {
        RIFFT1x16(&pOutput[i * 2 * nOutputStride], 2, &anTmp[i * RFFT_LENGTH]);
    }
}

#if !__hexagon__

void RFFT16x16_2x2(complex_t *pOutput, const uint16_t nOutputStride,
                   const real_t *pInput, const uint16_t nInputStride)
{
    assert(pOutput != NULL);
    assert(((uintptr_t)pOutput & (HVX_REG_LENGTH - 1)) == 0);
    assert(pInput != NULL);
    assert(((uintptr_t)pInput & (HVX_REG_LENGTH - 1)) == 0);
    assert(2 * 2 * 16 * sizeof(real_t) == HVX_REG_LENGTH);

    RFFT16x16_2_2(pOutput + 0 * nOutputStride + 0, nOutputStride, pInput + 0 * nInputStride + 0, nInputStride);
    RFFT16x16_2_2(pOutput + 0 * nOutputStride + 1, nOutputStride, pInput + 0 * nInputStride + 1, nInputStride);
    RFFT16x16_2_2(pOutput + 1 * nOutputStride + 0, nOutputStride, pInput + 1 * nInputStride + 0, nInputStride);
    RFFT16x16_2_2(pOutput + 1 * nOutputStride + 1, nOutputStride, pInput + 1 * nInputStride + 1, nInputStride);
}

#endif

#if !__hexagon__

void RIFFT16x16_2x2(real_t *pOutput, const uint16_t nOutputStride,
                    const complex_t *pInput, const uint16_t nInputStride)
{
    assert(pOutput != NULL);
    assert(((uintptr_t)pOutput & (HVX_REG_LENGTH - 1)) == 0);
    assert(pInput != NULL);
    assert(((uintptr_t)pInput & (HVX_REG_LENGTH - 1)) == 0);
    assert(2 * 2 * 16 * sizeof(real_t) == HVX_REG_LENGTH);

    RIFFT16x16_2_2(pOutput + 0 * nOutputStride + 0, nOutputStride, pInput + 0 * nInputStride + 0, nInputStride);
    RIFFT16x16_2_2(pOutput + 0 * nOutputStride + 1, nOutputStride, pInput + 0 * nInputStride + 1, nInputStride);
    RIFFT16x16_2_2(pOutput + 1 * nOutputStride + 0, nOutputStride, pInput + 1 * nInputStride + 0, nInputStride);
    RIFFT16x16_2_2(pOutput + 1 * nOutputStride + 1, nOutputStride, pInput + 1 * nInputStride + 1, nInputStride);
}

#endif

void RIFFT16x16_4_1(real_t *pOutput, const uint16_t nOutputStride,
                    const complex_t *pInput, const uint16_t nInputStride)
{

    for (int32_t j = 0; j < 16; j++)
    {
        for (int32_t i = 0; i < 9; i++)
        {
            // printf("RFFT1x4 out r%d%d = %d \n",i,j, pInput[i*9+j].r);
            // printf("RFFT1x4 out r%d%d = %d \n",i,j, pInput[j*9+i].r);
        }
    }

    /////////////////

    complex_t anTmp[16 * RFFT_LENGTH];

    for (int32_t i = 0; i < RFFT_LENGTH; i++)
    {
        CIFFT16x1(&anTmp[i], &pInput[i], nInputStride);
    }
    for (int32_t j = 0; j < 16; j++)
    {
        for (int32_t i = 0; i < 9; i++)
        {
            // printf("RFFT1x4 out r%d%d = %d \n",i,j, pInput[i*9+j].r);
            //	printf("RFFT1x4 out r%d%d = %d \n",i,j, anTmp[j*9+i].r);
        }
    }
    for (int32_t i = 0; i < 16; i++)
    {
        RIFFT1x16(&pOutput[i * 4 * nOutputStride], 2, &anTmp[i * RFFT_LENGTH]);
    }

    ///////////////
}
void RIFFT16x16_4_ref(real_t *pOutput, const uint16_t nOutputStride,
                      const complex_t *pInput, const uint16_t nInputStride)
{

    RIFFT16x16_4_1(pOutput + 0 * nOutputStride + 0, nOutputStride, pInput, nInputStride);
    RIFFT16x16_4_1(pOutput + 0 * nOutputStride + 1, nOutputStride, pInput + 16 * 9, nInputStride);
    RIFFT16x16_4_1(pOutput + 2 * nOutputStride + 0, nOutputStride, pInput + 16 * 9 * 2, nInputStride);
    RIFFT16x16_4_1(pOutput + 2 * nOutputStride + 1, nOutputStride, pInput + 16 * 9 * 3, nInputStride);
}
AEEResult qhci_IFFT16_16_ref(
    remote_handle64 handle,
    const int16_t *FFT_input, // Input point buffer of unsigned 16-bit values
    int pRawImgLen,
    int32_t stride_i,  // Stride of src
    int32_t width,     // Width of src
    int32_t height,    // Height of src
    int32_t outstride, // Stride of output
    int16_t *IFFT_out, // Pointer to the FFT output
    int FFT_outLen)
{
    for (int loops = 0; loops < LOOPS; loops++)
    {
        RIFFT16x16_4_ref(IFFT_out, 16, (complex_t *)FFT_input, 9);
    }
    return AEE_SUCCESS;
}

class FFT16_16 : public QhciFeatureBase
{
public:
    using QhciFeatureBase::QhciFeatureBase;
    int Test(remote_handle64 handle) override
    {
        AEEResult nErr = 0;
        // Api teat
        {
            // Step1. Define param && allocate buffer
            uint32_t pRawImgLen = 64 * 16;
            int16_t *pRawImg = (int16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, pRawImgLen * sizeof(int16_t));
            int32_t stride_i = 64;
            int32_t width = 64;
            int32_t height = 16;
            int32_t outstride = 128;
            uint32_t FFT_outLen = 64 * 16 * 2;
            int16_t *IFFT_out_cpu = (int16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, FFT_outLen * sizeof(int16_t));
            int16_t *FFT_out_dsp = (int16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, FFT_outLen * sizeof(int16_t));

            // Step2. Random generate input data, if need to use real data, pls comment this and implement by yourself.
            GenerateRandomData((uint8_t *)pRawImg, pRawImgLen * sizeof(int16_t));

            // Step3. DSP execute
            TIME_STAMP("DSP", qhci_FFT16_16(handle, pRawImg, pRawImgLen, stride_i, width, height, outstride, FFT_out_dsp, FFT_outLen));

            // Step4. CPU execute
            TIME_STAMP("Cpu", qhci_IFFT16_16_ref(handle, FFT_out_dsp, pRawImgLen, stride_i, width, height, outstride, IFFT_out_cpu, FFT_outLen));

            // Step5. Compare CPU & DSP results
            nErr = CompareBuffers(pRawImg, IFFT_out_cpu, pRawImgLen);
            CHECK(nErr == AEE_SUCCESS);

            // Step6. Free buffer
            if (pRawImg)
                rpcmem_free(pRawImg);
            if (IFFT_out_cpu)
                rpcmem_free(IFFT_out_cpu);
            if (FFT_out_dsp)
                rpcmem_free(FFT_out_dsp);
        }

        return nErr;
    }
};

static FFT16_16 *_feature = new FFT16_16(FEATURE_NAME);
static bool init = []
{
    QhciTest::GetInstance().current_map_func()[FEATURE_NAME] = _feature;
    return true;
}();
#undef FEATURE_NAME
