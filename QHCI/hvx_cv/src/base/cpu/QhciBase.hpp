//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#pragma once

#include <iostream>
#include <functional>
#include <cstdarg>
#include <random>
#include <cstring>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "verify.h"
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>

#include "AEEStdErr.h"
#include "rpcmem.h"
#include "qhci.h"
#include "os_defines.h"
#include "dsp_capabilities_utils.h"     // available under <HEXAGON_SDK_ROOT>/utils/examples
#include <sys/stat.h>
#include <fcntl.h>

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "remote.h"

//Set loop for fft and div
#define LOOPS 1

//ifft32x32,ifft64x64
typedef struct complex_s16_s
{
    short real;
    short imag;
} COMPLEX_S16_S;

typedef struct complex_s32_s
{
    int real;
    int imag;
} COMPLEX_S32_S;

#define CHECK(a)                            \
  if (!(a))                                 \
  {                                         \
    std::cout << #a " failed" << std::endl; \
    abort();                                \
  }

#define TIME_STAMP(str, pfunc) {\
        unsigned long long t1, t2; \
        t1 = get_time();\
        pfunc;\
        t2 = get_time();\
        printf("%s execute time: %llu us\n", str, t2-t1);\
}

class QhciFeatureBase {
public:
    std::string name;
    QhciFeatureBase(std::string feature) : name(feature) {};
    virtual ~QhciFeatureBase() {};
    virtual int Test(remote_handle64 handle);
    void GenerateRandomData(uint8_t* buffer, int size)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        for (int i = 0; i < size; i++) {
            buffer[i] = dis(gen);
        }
    }
    unsigned long long get_time()
    {
        struct timeval tv;
        gettimeofday(&tv, 0);
        return (tv.tv_sec * 1000000ULL + tv.tv_usec);
    }
    template <typename T>
    int CompareBuffers(const T* pCpu, const T* pDsp, int size)
    {
        int diffCount = 0;
        for (int i = 0; i < size; i++)
        {
            if (pCpu[i] != pDsp[i])
            {
                if (diffCount < 10)
                {
                    printf("Verify failed: pCpu[%d] = %d not match pDsp[%d] = %d\n", i, pCpu[i], i, pDsp[i]);
                }
                diffCount++;
            }
        }
        if(!diffCount) {
            std::cout << "Verify pass! DSP results same with CPU." << std::endl;
        }
        return diffCount;
    }
};

class QhciTest {
public:
    QhciTest();
    int QhciTestAll();
    int QhciTestSingle(std::string feature);
    std::unordered_map<std::string, QhciFeatureBase*>& current_map_func () {
        static std::unordered_map<std::string, QhciFeatureBase*> _map;
        return _map;
    }
    void MapRelease();
    void PringUsage();
    ~QhciTest();
    static QhciTest &GetInstance() {
        static QhciTest instance_;
        return instance_;
    }
    remote_handle64 handle = -1;
private:
};
