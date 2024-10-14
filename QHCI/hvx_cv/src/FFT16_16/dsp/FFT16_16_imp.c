//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

//==============================================================================
// Include Files
//==============================================================================
#include "qhci.h"
#include "AEEStdErr.h"
#include "HAP_power.h"
#include "HAP_farf.h"
#include "HAP_perf.h"
#include "remote.h"
#include "q6cache.h"
#include <assert.h>
#include "hexagon_types.h"
#include "hexagon_protos.h"
#include "HAP_vtcm_mgr.h"
#include "worker_pool.h"
#include "RIFFT16x16_4_asm.h"
#include <stdlib.h>
#include <string.h>

/*===========================================================================
    DEFINITIONS
===========================================================================*/
#define VLEN 128
#define AHEAD 1
// #define PROFILING_ON
typedef struct
{
    worker_synctoken_t *token; // worker pool token
    unsigned int rowsPerJob;   // number of rows to process per multi-threaded job
    // dspCV_hvx_config_t   *hvxInfo;          // HVX configuration information
    unsigned int threadCount; // thread counter shared by all workers
    int numThreads;
    int16 *src;             // source image pointer
    unsigned int srcWidth;  // source image width
    unsigned int height;    // number of output rows
    unsigned int srcStride; // source image stride
    int16 *dst;             // destination image pointer
    unsigned int dstStride; // destination image stride
    const char *mask;       // filter param
    int shift;              // filter param
} D2FFT_callback_t;

/*===========================================================================
    DECLARATIONS
===========================================================================*/
static void D2FFT_callback(
    void *data)
{
    D2FFT_callback_t *dptr = (D2FFT_callback_t *)data;
    int16 *debug_buf = (int16 *)memalign(VLEN, 16 * 128);
    // int16 * debug_buf = (int16 *) malloc(8*128);   ////for debug
    if (!debug_buf)
    {
        free(debug_buf);
    }
    memset(debug_buf, 0, 16 * 128);

    int16 *src = dptr->src;
    int16 *dst = dptr->dst;

    int fetch = 1;
    if (fetch == 1)
    {
        // initiate L2 prefetch
        long long L2FETCH_PARA = CreateL2pfParam(dptr->srcStride * sizeof(uint16), dptr->srcWidth * sizeof(uint16), 1, 0);
        L2fetch((unsigned int)(src + dptr->srcStride), L2FETCH_PARA);
        L2fetch((unsigned int)(dst + dptr->dstStride), L2FETCH_PARA);
    }

    for (int loops = 0; loops < LOOPS; loops++)
    {
        RIFFT16x16_4_asm(src, (const uint16_t)32, (complex_t *)dst, (const uint16_t)16, debug_buf);
    }
    if (debug_buf)
    {
        free(debug_buf);
    }

    // release multi-threading job token
    worker_pool_synctoken_jobdone(dptr->token);
}

/*===========================================================================
    GLOBAL FUNCTION
===========================================================================*/

AEEResult qhci_FFT16_16(
    remote_handle64 handle,
    const int16_t *pRawImg, // Input point buffer of unsigned 16-bit values
    int pRawImgLen,
    int32_t stride_i,  // Stride of src
    int32_t width,     // Width of src
    int32_t height,    // Height of src
    int32_t outstride, // Stride of output
    int16_t *FFT_out,  // Pointer to the FFT output
    int FFT_outLen)
{

// only supporting HVX version in this example.
#if (__HEXAGON_ARCH__ < 60)
    return AEE_EUNSUPPORTED;
#endif
#ifdef PROFILING_ON
    uint64 startTime = HAP_perf_get_time_us();
    uint64 startCycles = HAP_perf_get_pcycles();
#endif

    int numWorkers = num_hvx128_contexts;
    // numWorkers = 1;
    // split src image into horizontal stripes, for multi-threading.
    worker_pool_job_t job;
    worker_synctoken_t token;
    worker_pool_context_t *worker_pool_context = (worker_pool_context_t *)handle;
    // init the synchronization token for this dispatch.
    worker_pool_synctoken_init(&token, numWorkers);

    // init job function pointer
    job.fptr = D2FFT_callback;
    ;

    // init job data pointer. In this case, all the jobs can share the same copy of job data.
    D2FFT_callback_t dptr;
    dptr.token = &token;
    dptr.threadCount = 0;
    dptr.src = (int16 *)pRawImg;
    dptr.srcWidth = width;
    dptr.height = height;
    dptr.srcStride = stride_i;
    dptr.numThreads = numWorkers;
    dptr.dst = FFT_out;
    dptr.dstStride = outstride;
    // dptr.hvxInfo = &hvxInfo;
    // dptr.rowsPerJob = ((dptr.height + (numWorkers - 1)) / (numWorkers)) & -2;
    dptr.rowsPerJob = dptr.height;
    job.dptr = (void *)&dptr;

    unsigned int i;
    for (i = 0; i < numWorkers; i++)
    {
        // for multi-threaded impl, use this line.
        worker_pool_submit(*worker_pool_context, job);
    }
    worker_pool_synctoken_wait(&token);
#ifdef PROFILING_ON
    int32 dspUsec = 0;
    uint64 endCycles = HAP_perf_get_pcycles();
    uint64 endTime = HAP_perf_get_time_us();
    dspUsec = (int)(endTime - startTime);
    FARF(ALWAYS, "qhci_FFT16_16 profiling: %d PCycles, %d microseconds. Observed clock rate %d MHz",
         (int)(endCycles - startCycles), (int)(endTime - startTime), (int)((endCycles - startCycles) / (endTime - startTime)));
#endif

    return AEE_SUCCESS;
}
