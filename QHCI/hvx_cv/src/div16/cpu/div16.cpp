//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include "QhciBase.hpp"

#define FEATURE_NAME "div16"
// Step0. Define reference code here
AEEResult qhci_div16_integer_unsigned_ref(
    remote_handle64 handle,
    const uint16_t *srcDividend, // Input point buffer of unsigned 16-bit dividend
    uint32_t srcDividendLen,
    const uint16_t *srcDivisor, // Input point buffer of unsigned 16-bit divisor
    uint32_t srcDivisorLen,
    uint16_t *dst, // Pointer to the output
    uint32_t dstLen)
{
    for (int loops = 0; loops < LOOPS; loops++)
    {
        for (int i = 0; i < srcDividendLen; i++)
        {
            if (srcDivisor[i] == 0)
            {
                dst[i] = 0;
            }
            else
            {
                dst[i] = srcDividend[i] / srcDivisor[i];
            }
        }
    }
    return AEE_SUCCESS;
}

AEEResult qhci_div16_integer_signed_ref(
    remote_handle64 handle,
    const int16_t *srcDividend, // Input point buffer of signed 16-bit dividend
    uint32_t srcDividendLen,
    const int16_t *srcDivisor, // Input point buffer of signed 16-bit divisor
    uint32_t srcDivisorLen,
    int16_t *dst, // Pointer to the output
    uint32_t dstLen)
{
    for (int loops = 0; loops < LOOPS; loops++)
    {
        for (int i = 0; i < srcDividendLen; i++)
        {
            if (srcDivisor[i] == 0 && srcDividend[i] < 0)
            {
                dst[i] = -32767;
            }
            else if (srcDivisor[i] == 0 && srcDividend[i] >= 0)
            {
                dst[i] = 32767;
            }
            else
            {
                dst[i] = srcDividend[i] / srcDivisor[i];
            }
        }
    }
    return AEE_SUCCESS;
}

AEEResult qhci_div16_fractional_ref(
    remote_handle64 handle,
    const uint16_t *srcDividend, // Input point buffer of unsigned 16-bit dividend
    uint32_t srcDividendLen,
    const uint16_t *srcDivisor, // Input point buffer of unsigned 16-bit divisor
    uint32_t srcDivisorLen,
    uint16_t *dst, // Pointer to the output
    uint32_t dstLen)
{
    for (int loops = 0; loops < LOOPS; loops++)
    {
        for (int i = 0; i < srcDividendLen; i++)
        {
            if (srcDivisor[i] == 0)
            {
                dst[i] = 32767;
            }
            else
            {
                dst[i] = (uint16_t)(((float)srcDividend[i] / (float)srcDivisor[i]) * 32767);
            }
        }
    }
    return AEE_SUCCESS;
}

int random_int(int min, int max)
{
    return min + rand() % (max - min + 1);
}

class DIV16 : public QhciFeatureBase
{
public:
    using QhciFeatureBase::QhciFeatureBase;
    int Test(remote_handle64 handle) override
    {
        AEEResult nErr = 0;
        // Api teat
        {
            printf("start qhci_div16_integer_unsigned\n");
            // Step1. Define param && allocate buffer
            uint32_t srcDividendLen = 12800;
            uint16_t *srcDividend = (uint16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, srcDividendLen * sizeof(uint16_t));
            uint32_t srcDivisorLen = srcDividendLen;
            uint16_t *srcDivisor = (uint16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, srcDivisorLen * sizeof(uint16_t));
            uint32_t dstLen = srcDividendLen;
            uint16_t *dst_cpu = (uint16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, dstLen * sizeof(uint16_t));
            uint16_t *dst_dsp = (uint16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, dstLen * sizeof(uint16_t));

            // Step2. Random generate input data, if need to use real data, pls comment this and implement by yourself.
            srand(time(NULL));
            int min = 0;
            int max = INT16_MAX;
            for (int i = 0; i < dstLen; i++)
            {
                srcDividend[i] = random_int(min, max);
                srcDivisor[i] = random_int(min, max);
            }

            // Step3. CPU execute
            TIME_STAMP("Cpu", qhci_div16_integer_unsigned_ref(handle, srcDividend, srcDividendLen, srcDivisor, srcDivisorLen, dst_cpu, dstLen));

            // Step4. DSP execute
            TIME_STAMP("DSP", qhci_div16_integer_unsigned(handle, srcDividend, srcDividendLen, srcDivisor, srcDivisorLen, dst_dsp, dstLen));

            // Step5. Compare CPU & DSP results
            nErr = CompareBuffers(dst_cpu, dst_dsp, dstLen);
            CHECK(nErr == AEE_SUCCESS);

            // Step6. Free buffer
            if (srcDividend)
                rpcmem_free(srcDividend);
            if (srcDivisor)
                rpcmem_free(srcDivisor);
            if (dst_cpu)
                rpcmem_free(dst_cpu);
            if (dst_dsp)
                rpcmem_free(dst_dsp);
        }
        {
            printf("start qhci_div16_integer_signed\n");
            // Step1. Define param && allocate buffer
            uint32_t srcDividendLen = 12800;
            int16_t *srcDividend = (int16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, srcDividendLen * sizeof(int16_t));
            uint32_t srcDivisorLen = srcDividendLen;
            int16_t *srcDivisor = (int16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, srcDivisorLen * sizeof(int16_t));
            uint32_t dstLen = srcDividendLen;
            int16_t *dst_cpu = (int16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, dstLen * sizeof(int16_t));
            int16_t *dst_dsp = (int16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, dstLen * sizeof(int16_t));

            // Step2. Random generate input data, if need to use real data, pls comment this and implement by yourself.
            srand(time(NULL));
            int min = INT16_MIN + 1;
            int max = INT16_MAX;
            srcDividend[0] = 0;
            srcDivisor[0] = 0;
            for (int i = 1; i < dstLen; i++)
            {
                srcDividend[i] = random_int(min, max);
                srcDivisor[i] = random_int(min, max);
            }

            // Step3. CPU execute
            TIME_STAMP("Cpu", qhci_div16_integer_signed_ref(handle, srcDividend, srcDividendLen, srcDivisor, srcDivisorLen, dst_cpu, dstLen));

            // Step4. DSP execute
            TIME_STAMP("DSP", qhci_div16_integer_signed(handle, srcDividend, srcDividendLen, srcDivisor, srcDivisorLen, dst_dsp, dstLen));

            // Step5. Compare CPU & DSP results
            nErr = CompareBuffers(dst_cpu, dst_dsp, dstLen);
            CHECK(nErr == AEE_SUCCESS);

            // Step6. Free buffer
            if (srcDividend)
                rpcmem_free(srcDividend);
            if (srcDivisor)
                rpcmem_free(srcDivisor);
            if (dst_cpu)
                rpcmem_free(dst_cpu);
            if (dst_dsp)
                rpcmem_free(dst_dsp);
        }
        {
            printf("start qhci_div16_fractional\n");
            // Step1. Define param && allocate buffer
            uint32_t srcDividendLen = 12800;
            uint16_t *srcDividend = (uint16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, srcDividendLen * sizeof(uint16_t));
            uint32_t srcDivisorLen = srcDividendLen;
            uint16_t *srcDivisor = (uint16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, srcDivisorLen * sizeof(uint16_t));
            uint32_t dstLen = srcDividendLen;
            uint16_t *dst_cpu = (uint16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, dstLen * sizeof(uint16_t));
            uint16_t *dst_dsp = (uint16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, dstLen * sizeof(uint16_t));

            // Step2. Random generate input data, if need to use real data, pls comment this and implement by yourself.
            srand(time(NULL));
            int min = 0;
            int max = UINT16_MAX;
            for (int i = 0; i < dstLen; i++)
            {
                srcDividend[i] = random_int(min, max);
                srcDivisor[i] = random_int(srcDividend[i], max);
            }

            // Step3. CPU execute
            TIME_STAMP("Cpu", qhci_div16_fractional_ref(handle, srcDividend, srcDividendLen, srcDivisor, srcDivisorLen, dst_cpu, dstLen));

            // Step4. DSP execute
            TIME_STAMP("DSP", qhci_div16_fractional(handle, srcDividend, srcDividendLen, srcDivisor, srcDivisorLen, dst_dsp, dstLen));

            // Step5. Compare CPU & DSP results
            int bitexactErrors = 0;
            for (int i = 0; i < dstLen; i++)
            {
                if (abs(dst_cpu[i] - dst_dsp[i]) > 1)
                {
                    bitexactErrors++;
                    if (bitexactErrors < 10)
                        printf("Bit exact error: Dividend=%d, Divisor=%d, dst_cpu=%d, dst_dsp=%d\n", srcDividend[i], srcDivisor[i], dst_cpu[i], dst_dsp[i]);
                }
            }
            if (!bitexactErrors)
            {
                std::cout << "Verify pass! DSP results same with CPU." << std::endl;
            }
            CHECK(0 == bitexactErrors);
            // Step6. Free buffer
            if (srcDividend)
                rpcmem_free(srcDividend);
            if (srcDivisor)
                rpcmem_free(srcDivisor);
            if (dst_cpu)
                rpcmem_free(dst_cpu);
            if (dst_dsp)
                rpcmem_free(dst_dsp);
        }

        return nErr;
    }
};

static DIV16 *_feature = new DIV16(FEATURE_NAME);
static bool init = []
{
    QhciTest::GetInstance().current_map_func()[FEATURE_NAME] = _feature;
    return true;
}();
#undef FEATURE_NAME
