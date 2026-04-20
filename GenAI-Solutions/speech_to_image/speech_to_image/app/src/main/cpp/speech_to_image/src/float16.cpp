//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include "float16.hpp"

float16 get_16bit_float(float n) {
    uint32_t intn = *reinterpret_cast<uint32_t*>(&n);


    uint16_t sign = (intn >> 16) & 0x8000;

    int32_t exponent = ((intn >> 23) & 0xFF) - 127 + 15;
    if (exponent <= 0) {
        exponent = 0;
    } else if (exponent >= 31) {
        exponent = 31;
    }
    exponent = (exponent & 0x1F) << 10;

    uint16_t mantissa = (intn >> 13) & 0x3FF;

    uint16_t result = sign | exponent | mantissa;

    return result;
}

float get_32bit_float(float16 n) {
    uint32_t sign = (n & 0x8000) << 16;
    uint32_t exponent = (((n & 0x7C00) >> 10) + 127 - 15) << 23;
    uint32_t mantissa = (n & 0x3FF) << 13;

    uint32_t representation32bits = sign | exponent | mantissa;

    float result = *reinterpret_cast<float*>(&representation32bits);

    return result;
}