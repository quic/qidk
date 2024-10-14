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

/*===========================================================================
    DEFINITIONS
===========================================================================*/
extern void div_integer_signed(int16 *SumF, int16 *Denome, int16 *imgDst, unsigned int srcLen);
extern void div_integer_denome0(uint16 *SumF, uint16 *Denome, uint16 *imgDst, unsigned int srcLen);
extern void div_fractional(uint16 *SumF, uint16 *Denome, uint16 *imgDst, unsigned int srcLen);

typedef struct
{
    worker_synctoken_t *token; // worker pool token
    unsigned int jobCount;     // atomic counter shared by all workers
    uint16 *SumF;
    uint16 *Denome;        // source image pointer
    unsigned int srcWidth; // source image width

    uint16 *dst; // destination image pointer
                 // number of rows to process per multi-threaded job
    // dspCV_hvx_config_t   *hvxInfo;          // HVX configuration information
} div_integer_denome0_callback_t;

static void div_integer_denome0_callback(void *data)
{
    div_integer_denome0_callback_t *dptr = (div_integer_denome0_callback_t *)data;
    uint16 *SumF = dptr->SumF;
    uint16 *Denome = dptr->Denome;
    uint16 *imgDst = dptr->dst;
    unsigned int srcLen = dptr->srcWidth;

    int fetch = 1;
    if (fetch == 1)
    {
        // initiate L2 prefetch
        long long L2FETCH_PARA = CreateL2pfParam(dptr->srcWidth, dptr->srcWidth, 16, 0);
        L2fetch((unsigned int)(SumF), L2FETCH_PARA);
        L2fetch((unsigned int)(Denome), L2FETCH_PARA);
        L2fetch((unsigned int)(imgDst), L2FETCH_PARA);
    }
    for (int loops = 0; loops < LOOPS; loops++)
    {
        div_integer_denome0(SumF, Denome, imgDst, srcLen); // *SumF / *Denome
    }
    // release multi-threading job token
    worker_pool_synctoken_jobdone(dptr->token);
}

typedef struct
{
    worker_synctoken_t *token; // worker pool token
    unsigned int jobCount;     // atomic counter shared by all workers
    int16 *SumF;
    int16 *Denome;         // source image pointer
    unsigned int srcWidth; // source image width
    int16 *dst;            // destination image pointer
                           // dspCV_hvx_config_t   *hvxInfo;          // HVX configuration information
} div_integer_signed_callback_t;

static void div_integer_signed_callback(void *data)
{
    div_integer_signed_callback_t *dptr = (div_integer_signed_callback_t *)data;
    // loop until no more horizontal stripes to process
    int16 *SumF = dptr->SumF;
    int16 *Denome = dptr->Denome;
    int16 *imgDst = dptr->dst;
    unsigned int srcLen = dptr->srcWidth;

    int fetch = 1;
    if (fetch == 1)
    {
        // initiate L2 prefetch
        long long L2FETCH_PARA = CreateL2pfParam(dptr->srcWidth, dptr->srcWidth, 16, 0);
        L2fetch((unsigned int)(SumF), L2FETCH_PARA);
        L2fetch((unsigned int)(Denome), L2FETCH_PARA);
        L2fetch((unsigned int)(imgDst), L2FETCH_PARA);
    }
    for (int loops = 0; loops < LOOPS; loops++)
    {
        div_integer_signed(SumF, Denome, imgDst, srcLen); // *SumF / *Denome
    }
    // release multi-threading job token
    worker_pool_synctoken_jobdone(dptr->token);
}

typedef struct
{
    worker_synctoken_t *token; // worker pool token
    unsigned int jobCount;     // atomic counter shared by all workers
    uint16 *SumF;
    uint16 *Denome;        // source image pointer
    unsigned int srcWidth; // source image width
    uint16 *dst;           // destination image pointer
                           // number of rows to process per multi-threaded job
    // dspCV_hvx_config_t   *hvxInfo;          // HVX configuration information
} div_fractional_callback_t;

static void div_fractional_callback(void *data)
{
    div_fractional_callback_t *dptr = (div_fractional_callback_t *)data;
    // loop until no more horizontal stripes to process
    uint16 *SumF = dptr->SumF;
    uint16 *Denome = dptr->Denome;
    uint16 *imgDst = dptr->dst;
    unsigned int srcLen = dptr->srcWidth;

    int fetch = 1;
    if (fetch == 1)
    {
        // initiate L2 prefetch
        long long L2FETCH_PARA = CreateL2pfParam(dptr->srcWidth, dptr->srcWidth, 16, 0);
        L2fetch((unsigned int)(SumF), L2FETCH_PARA);
        L2fetch((unsigned int)(Denome), L2FETCH_PARA);
        L2fetch((unsigned int)(imgDst), L2FETCH_PARA);
    }
    for (int loops = 0; loops < LOOPS; loops++)
    {
        div_fractional(SumF, Denome, imgDst, srcLen); // *SumF / *Denome
    }
    // release multi-threading job token
    worker_pool_synctoken_jobdone(dptr->token);
}

/*===========================================================================
    DECLARATIONS
===========================================================================*/

/*===========================================================================
    GLOBAL FUNCTION
===========================================================================*/

AEEResult qhci_div16_integer_unsigned(
    remote_handle64 handle,
    const uint16_t *srcDividend, // Input point buffer of unsigned 16-bit dividend
    int srcDividendLen,
    const uint16_t *srcDivisor, // Input point buffer of unsigned 16-bit divisor
    int srcDivisorLen,
    uint16_t *dst, // Pointer to the output
    int dstLen)
{

    // record start time (in both microseconds and pcycles) for profiling
#ifdef PROFILING_ON
    uint64 startTime = HAP_perf_get_time_us();
    uint64 startCycles = HAP_perf_get_pcycles();
#endif
    int numWorkers = num_hvx128_contexts;
    worker_pool_job_t job;
    worker_synctoken_t token;
    worker_pool_context_t *worker_pool_context = (worker_pool_context_t *)handle;

    // init the synchronization token for this dispatch.
    worker_pool_synctoken_init(&token, numWorkers);

    // init job function pointer
    job.fptr = div_integer_denome0_callback;

    // init job data pointer. In this case, all the jobs can share the same copy of job data.
    div_integer_denome0_callback_t dptr;
    dptr.token = &token;
    dptr.jobCount = 0;
    dptr.SumF = (uint16 *)srcDividend;
    dptr.Denome = (uint16 *)srcDivisor;
    dptr.dst = dst;
    dptr.srcWidth = (unsigned int)srcDividendLen;
    job.dptr = (void *)&dptr;

    unsigned int i;
    for (i = 0; i < numWorkers; i++)
    {
        worker_pool_submit(*worker_pool_context, job);
    }
    worker_pool_synctoken_wait(&token);
#ifdef PROFILING_ON
    int32 profResult = 0;
    uint64 endCycles = HAP_perf_get_pcycles();
    uint64 endTime = HAP_perf_get_time_us();
    profResult = (uint32)(endTime - startTime);
    FARF(ALWAYS, "qhci_div16_integer_unsigned profiling: %d PCycles, %d microseconds. Observed clock rate %d MHz",
         (int)(endCycles - startCycles), (int)(endTime - startTime), (int)((endCycles - startCycles) / (endTime - startTime)));
#endif

    return AEE_SUCCESS;
}
AEEResult qhci_div16_integer_signed(
    remote_handle64 handle,
    const int16_t *srcDividend, // Input point buffer of signed 16-bit dividend
    int srcDividendLen,
    const int16_t *srcDivisor, // Input point buffer of signed 16-bit divisor
    int srcDivisorLen,
    int16_t *dst, // Pointer to the output
    int dstLen)
{
#ifdef PROFILING_ON
    uint64 startTime = HAP_perf_get_time_us();
    uint64 startCycles = HAP_perf_get_pcycles();
#endif
    int numWorkers = num_hvx128_contexts;
    // numWorkers = 1;
    // numWorkers split src image into horizontal stripes, for multi-threading.
    worker_pool_job_t job;
    worker_synctoken_t token;
    worker_pool_context_t *worker_pool_context = (worker_pool_context_t *)handle;

    // init the synchronization token for this dispatch.
    worker_pool_synctoken_init(&token, numWorkers);

    // init job function pointer
    job.fptr = div_integer_signed_callback;

    // init job data pointer. In this case, all the jobs can share the same copy of job data.
    div_integer_signed_callback_t dptr;
    dptr.token = &token;
    dptr.jobCount = 0;
    dptr.SumF = (int16 *)srcDividend;
    dptr.Denome = (int16 *)srcDivisor;
    dptr.dst = dst;
    dptr.srcWidth = srcDividendLen;

    // dptr.hvxInfo = &hvxInfo;
    job.dptr = (void *)&dptr;

    unsigned int i;
    for (i = 0; i < numWorkers; i++)
    {
        // for multi-threaded impl, use this line.
        worker_pool_submit(*worker_pool_context, job);

        // This line can be used instead of the above to directly invoke the
        // callback function without dispatching to the worker pool. Useful
        // to avoid multi-threading in debug scenarios to narrow down problems.
        //        job.fptr(job.dptr);
    }
    worker_pool_synctoken_wait(&token);
    // record end time (in both microseconds and pcycles) for profiling
#ifdef PROFILING_ON
    int32 profResult = 0;
    uint64 endCycles = HAP_perf_get_pcycles();
    uint64 endTime = HAP_perf_get_time_us();
    profResult = (uint32)(endTime - startTime);
    FARF(ALWAYS, "qhci_div16_integer_signed profiling: %d PCycles, %d microseconds. Observed clock rate %d MHz",
         (int)(endCycles - startCycles), (int)(endTime - startTime), (int)((endCycles - startCycles) / (endTime - startTime)));
#endif
    return AEE_SUCCESS;
}
AEEResult qhci_div16_fractional(
    remote_handle64 handle,
    const uint16_t *srcDividend, // Input point buffer of unsigned 16-bit dividend
    int srcDividendLen,
    const uint16_t *srcDivisor, // Input point buffer of unsigned 16-bit divisor
    int srcDivisorLen,
    uint16_t *dst, // Pointer to the output
    int dstLen)
{
#ifdef PROFILING_ON
    uint64 startTime = HAP_perf_get_time_us();
    uint64 startCycles = HAP_perf_get_pcycles();
#endif
    int numWorkers = num_hvx128_contexts;
    // numWorkers = 1;
    // numWorkers split src image into horizontal stripes, for multi-threading.
    worker_pool_job_t job;
    worker_synctoken_t token;
    worker_pool_context_t *worker_pool_context = (worker_pool_context_t *)handle;

    // init the synchronization token for this dispatch.
    worker_pool_synctoken_init(&token, numWorkers);

    // init job function pointer
    job.fptr = div_fractional_callback;

    // init job data pointer. In this case, all the jobs can share the same copy of job data.
    div_fractional_callback_t dptr;
    dptr.token = &token;
    dptr.jobCount = 0;
    dptr.SumF = (uint16 *)srcDividend;
    dptr.Denome = (uint16 *)srcDivisor;
    dptr.dst = dst;
    dptr.srcWidth = srcDividendLen;
    // Stripe image to balance load across available threads, aiming for 3 stripes per
    // worker, making sure rowsPerJob is multiple of 2

    // dptr.hvxInfo = &hvxInfo;
    job.dptr = (void *)&dptr;

    unsigned int i;
    for (i = 0; i < numWorkers; i++)
    {
        // for multi-threaded impl, use this line.
        worker_pool_submit(*worker_pool_context, job);

        // This line can be used instead of the above to directly invoke the
        // callback function without dispatching to the worker pool. Useful
        // to avoid multi-threading in debug scenarios to narrow down problems.
        //        job.fptr(job.dptr);
    }
    worker_pool_synctoken_wait(&token);

    // clean up hvx configuration - release temporary reservation (if any), turn off power, etc.
    // dspCV_hvx_cleanup_mt_job(&hvxInfo);
    // record end time (in both microseconds and pcycles) for profiling
#ifdef PROFILING_ON
    int32 profResult = 0;
    uint64 endCycles = HAP_perf_get_pcycles();
    uint64 endTime = HAP_perf_get_time_us();
    profResult = (uint32)(endTime - startTime);
    FARF(ALWAYS, "qhci_div16_fractional profiling: %d PCycles, %d microseconds. Observed clock rate %d MHz",
         (int)(endCycles - startCycles), (int)(endTime - startTime), (int)((endCycles - startCycles) / (endTime - startTime)));
#endif
    return AEE_SUCCESS;
}
