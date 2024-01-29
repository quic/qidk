// -*- mode: cpp -*-
// =============================================================================
// @@-COPYRIGHT-START-@@
//
// Copyright (c) 2023 of Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
//
// @@-COPYRIGHT-END-@@
// =============================================================================
#pragma once
#ifndef UTILS_H_
#define UTILS_H_

#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>

using namespace std;

#define PRINT(fmt, ...) { \
    printf(fmt, ##__VA_ARGS__); \
}

#define LOG(level, fmt, ...) { \
    PRINT("[%s] - %s: " fmt, #level, __func__, ##__VA_ARGS__); \
}

//#define DEBUG
#ifdef DEBUG
#define LOG_DEBUG(fmt, ...)   LOG(DEBUG, fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)  ((void)0)
#endif

#define LOG_INFO(fmt, ...) { \
    LOG(INFO, fmt, ##__VA_ARGS__); \
}

#define LOG_WARN(fmt, ...) { \
    LOG(WARN, fmt, ##__VA_ARGS__); \
}

#define LOG_ERROR(fmt, ...) { \
    LOG(ERROR, fmt, ##__VA_ARGS__); \
}

// Inference hardware runtime.
typedef enum runtime {
    CPU = 0,
    DSP
}runtime_t;

typedef enum PerformanceProfile {
    DEFAULT = 0,
    /// Run in a balanced mode.
    BALANCED = 0,
    /// Run in high performance mode
    HIGH_PERFORMANCE = 1,
    /// Run in a power sensitive mode, at the expense of performance.
    POWER_SAVER = 2,
    /// Use system settings.  SNPE makes no calls to any performance related APIs.
    SYSTEM_SETTINGS = 3,
    /// Run in sustained high performance mode
    SUSTAINED_HIGH_PERFORMANCE = 4,
    /// Run in burst mode
    BURST = 5,
    /// Run in lower clock than POWER_SAVER, at the expense of performance.
    LOW_POWER_SAVER = 6,
    /// Run in higher clock and provides better performance than POWER_SAVER.
    HIGH_POWER_SAVER = 7,
    /// Run in lower balanced mode
    LOW_BALANCED = 8,
    //Run in extreme power saver mode
    EXTREME_POWERSAVER = 9
}performance_t;

template <class T>
void ClearVector(std::vector<T>& vt)
{
    std::vector<T> vtTemp;
    vtTemp.swap(vt);
}

#endif