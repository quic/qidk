//=============================================================================
//
//  Copyright (c) 2022 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//=============================================================================

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
// #define PROFILING_ON

/*===========================================================================
    DECLARATIONS
===========================================================================*/

/*===========================================================================
    TYPEDEF
===========================================================================*/

void Gaussian5x5u16PerRow_intrinsic(
    uint16_t **pSrc,
    int width,
    uint16_t *dst,
    int VLEN);

// multi-threading context structure. This structure contains everything
// the multi-threaded callback needs to process the right portion of the
// input image.
typedef struct
{
    worker_synctoken_t *token; // worker pool token
    unsigned int jobCount;     // atomic counter shared by all workers
    uint16 *src;               // source image pointer
    unsigned int srcWidth;     // source image width
    unsigned int height;       // number of output rows
    unsigned int srcStride;    // source image stride
    uint16 *dst;               // destination image pointer
    unsigned int dstStride;    // destination image stride
    unsigned int rowsPerJob;   // number of rows to process per multi-threaded job
    // dspCV_hvx_config_t   *hvxInfo;          // HVX configuration information
} gaussian5x5_callback_t;

/*===========================================================================
    LOCAL FUNCTION
===========================================================================*/
// multi-threading callback function
static void gaussian5x5_callback(void *data)
{
    gaussian5x5_callback_t *dptr = (gaussian5x5_callback_t *)data;
    // loop until no more horizontal stripes to process
    while (1)
    {
        // atomically add 1 to the job count to claim a stripe.
        unsigned int jobCount = worker_pool_atomic_inc_return(&(dptr->jobCount)) - 1;

        // if all horizontal stripes have been claimed for processing, break out and exit the callback
        if (jobCount * dptr->rowsPerJob >= dptr->height)
        {
            break;
        }

        // Set pointers to appropriate line of image for this stripe
        uint16 *src = dptr->src + (dptr->srcStride * dptr->rowsPerJob * jobCount);
        uint16 *dst = dptr->dst + (dptr->dstStride * dptr->rowsPerJob * jobCount);

        // initiate L2 prefetch (first 7 rows)
        long long L2FETCH_PARA = CreateL2pfParam(dptr->srcStride * sizeof(uint16), dptr->srcWidth * sizeof(uint16), 5, 0);
        L2fetch((unsigned int)src, L2FETCH_PARA);
        // next prefetches will just add 1 row
        L2FETCH_PARA = CreateL2pfParam(dptr->srcStride * sizeof(uint16), dptr->srcWidth * sizeof(uint16), 1, 0);

        uint16 *pSrc[5];
        int i;
        for (i = 0; i < 5; i++)
        {
            pSrc[i] = src + i * dptr->srcStride;
        }

        // find height of this stripe. Usually dptr->rowsPerJob, except possibly for the last stripe.
        unsigned int remainingRows = dptr->height - (dptr->rowsPerJob * jobCount);
        unsigned int height = (remainingRows < dptr->rowsPerJob) ? remainingRows : dptr->rowsPerJob;

        // HVX-optimized implementation
        for (i = 0; i < height; i++)
        {
            // update pointers
            if (i > 0)
            {
                int j;
                for (j = 0; j < 4; j++)
                    pSrc[j] = pSrc[j + 1];
                pSrc[4] = pSrc[3] + dptr->srcStride;
                dst += dptr->dstStride;
            }
            // fetch next row
            if (i + 1 < height)
            {
                L2fetch((unsigned int)(pSrc[4] + dptr->srcStride), L2FETCH_PARA);
            }
            for (int loops = 0; loops < LOOPS; loops++)
            {
            Gaussian5x5u16PerRow_intrinsic(pSrc, dptr->srcWidth, dst, 128);
            }
        }
    }
    // release multi-threading job token
    worker_pool_synctoken_jobdone(dptr->token);
}
/*===========================================================================
    GLOBAL FUNCTION
===========================================================================*/

AEEResult qhci_gaussian5x5(
    remote_handle64 handle,
    const uint16_t *src, // source buffer
    int srcLen,
    uint32_t srcWidth,  // source width
    uint32_t srcHeight, // source height
    uint32_t srcStride, // source stride
    uint32_t dstStride, // dst stride
    uint16_t *dst,      // output dst buffer
    int dstLen)
{
// only supporting HVX version in this example.
#if (__HEXAGON_ARCH__ < 60)
    return AEE_EUNSUPPORTED;
#endif

    // record start time (in both microseconds and pcycles) for profiling
#ifdef PROFILING_ON
    uint64 startTime = HAP_perf_get_time_us();
    uint64 startCycles = HAP_perf_get_pcycles();
#endif
    // Only supporting 128-byte aligned!!
    if (!(src && dst && ((((uint32)src | (uint32)dst | srcWidth | srcStride | dstStride) & 127) == 0) && (srcHeight >= 5)))
    {
        return AEE_EBADPARM;
    }
    int numWorkers = num_hvx128_contexts;
    // numWorkers=1;
    // split src image into horizontal stripes, for multi-threading.
    worker_pool_job_t job;
    worker_synctoken_t token;
    worker_pool_context_t *worker_pool_context = (worker_pool_context_t *)handle;

    // init the synchronization token for this dispatch.
    worker_pool_synctoken_init(&token, numWorkers);

    // init job function pointer
    job.fptr = gaussian5x5_callback;

    // init job data pointer. In this case, all the jobs can share the same copy of job data.
    gaussian5x5_callback_t dptr;
    dptr.token = &token;
    dptr.jobCount = 0;
    dptr.src = (uint16 *)src;
    dptr.srcWidth = srcWidth;
    dptr.height = (srcHeight - 4);
    dptr.srcStride = srcStride;
    dptr.dst = dst + (2 * dstStride);
    dptr.dstStride = dstStride;
    // Stripe image to balance load across available threads, aiming for 3 stripes per
    // worker, making sure rowsPerJob is multiple of 2
    dptr.rowsPerJob = (dptr.height + (3 * numWorkers - 1)) / (3 * numWorkers);
    // dptr.hvxInfo = &hvxInfo;
    job.dptr = (void *)&dptr;

    unsigned int i;
    for (i = 0; i < numWorkers; i++)
    {
        // for multi-threaded impl, use this line.
        worker_pool_submit(*worker_pool_context, job);
    }
    worker_pool_synctoken_wait(&token);
    // record end time (in both microseconds and pcycles) for profiling
#ifdef PROFILING_ON
    uint64 endCycles = HAP_perf_get_pcycles();
    uint64 endTime = HAP_perf_get_time_us();
    FARF(ALWAYS, "qhci_gaussian5x5 profiling: %d PCycles, %d microseconds. Observed clock rate %d MHz",
         (int)(endCycles - startCycles), (int)(endTime - startTime), (int)((endCycles - startCycles) / (endTime - startTime)));
#endif

    return AEE_SUCCESS;
}
