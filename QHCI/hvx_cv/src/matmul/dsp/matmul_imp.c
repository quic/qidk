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
#ifdef PROFILING_ON
#undef PROFILING_ON
#endif
//#define PROFILING_ON


/*===========================================================================
    DECLARATIONS
===========================================================================*/
typedef struct
{
    worker_synctoken_t *token; // worker pool token
    unsigned int rowsPerJob;   // number of rows to process per multi-threaded job
    unsigned int numThreads;   // number of worker threads
    unsigned int jobCount;     // thread counter shared by all workers
    uint8_t *mIn0;        // left matrix
    uint8_t *mIn1;        // right matrix
    uint32_t *mOut;        // mOut matrix
    uint32_t M; //mIn0 row
    uint32_t K; //mIn0 col/mIn1 row
    uint32_t N; //mIn1 col
} matmul_int8_callback_t;

/*===========================================================================
    GLOBAL FUNCTION
===========================================================================*/
static void matmul_int8_callback(
    void *data)
{
    matmul_int8_callback_t *dptr = (matmul_int8_callback_t *)data;
    // loop until no more horizontal stripes to process
    while (1)
    {
        // atomically add 1 to the job count to claim a stripe.
        unsigned int jobCount = worker_pool_atomic_inc_return(&(dptr->jobCount)) - 1;
        // if all horizontal stripes have been claimed for processing, break out and exit the callback
        if (jobCount * dptr->rowsPerJob >= dptr->M)
        {
            break;
        }
        // Set pointers to appropriate line of image for this stripe
        uint8_t *mIn0 = dptr->mIn0 + dptr->rowsPerJob * dptr->K * jobCount;
        uint8_t *mIn1 = dptr->mIn1;
        uint32_t *mOut = dptr->mOut + dptr->rowsPerJob * dptr->N * jobCount;


        long long L2FETCH_PARA = CreateL2pfParam(dptr->N*4, dptr->N*4, 1, 0);
        L2fetch((unsigned int)mIn1, L2FETCH_PARA);

        unsigned int remainingRows = dptr->M - (dptr->rowsPerJob * jobCount);
        unsigned int M = (remainingRows < dptr->rowsPerJob) ? remainingRows : dptr->rowsPerJob;
        unsigned int K = dptr->K;
        unsigned int N = dptr->N;

        HVX_Vector v0, v1, v2, v3, sum0, sum1, sum2, sum3;
        HVX_VectorPair vD0, vD1, vD3, vD4;
        HVX_Vector *pIn1_0, *pIn1_1, *pIn1_2, *pIn1_3, *pOut;
        uint32_t *pIn0;
        uint8_t* pIn1;

        for (int m=0; m<M; m++) {
            pIn0 = (uint32_t*)(mIn0 + m*K);
            for(int k=0; k<K/4; k++) {
                pIn1 = mIn1 + k*4*dptr->N;
                pOut = (HVX_Vector*) (mOut + m*N);
                pIn1_0 = (HVX_Vector*) (pIn1);
                pIn1_1 = (HVX_Vector*) (pIn1 + dptr->N);
                pIn1_2 = (HVX_Vector*) (pIn1 + 2*dptr->N);
                pIn1_3 = (HVX_Vector*) (pIn1 + 3*dptr->N);

                if (k + 1 < K/4)
                {
                    L2fetch((unsigned int)(pIn1_3 + dptr->N), L2FETCH_PARA);
                }

                for (int n=0; n < N/128; n++) {
                    if(k == 0) {
                        sum0 = Q6_V_vzero();
                        sum1 = Q6_V_vzero();
                        sum2 = Q6_V_vzero();
                        sum3 = Q6_V_vzero();
                    } else {
                        sum0 = *pOut;
                        sum1 = *(pOut+1);
                        sum2 = *(pOut+2);
                        sum3 = *(pOut+3);
                    }
                    v0 = *pIn1_0++;
                    v1 = *pIn1_1++;
                    v2 = *pIn1_2++;
                    v3 = *pIn1_3++;

                    vD0 = Q6_W_vshuff_VVR(v1, v0, -1); //0,0,1,1,
                    vD1 = Q6_W_vshuff_VVR(v3, v2, -1); //0,0,1,1,
                    vD3 = Q6_W_vshuff_VVR(Q6_V_lo_W(vD1), Q6_V_lo_W(vD0), -2); //0,0,0,0,
                    vD4 = Q6_W_vshuff_VVR(Q6_V_hi_W(vD1), Q6_V_hi_W(vD0), -2); //0,0,1,1,

                    *pOut++ = Q6_Vuw_vrmpyacc_VuwVubRub(sum0,  Q6_V_lo_W(vD3), *pIn0); // 0~31
                    *pOut++ = Q6_Vuw_vrmpyacc_VuwVubRub(sum1,  Q6_V_hi_W(vD3), *pIn0); // 32~63
                    *pOut++ = Q6_Vuw_vrmpyacc_VuwVubRub(sum2,  Q6_V_lo_W(vD4), *pIn0); // 64~95
                    *pOut++ = Q6_Vuw_vrmpyacc_VuwVubRub(sum3,  Q6_V_hi_W(vD4), *pIn0); // 96~127
                }
                pIn0++;
            }
        }
    }
    // release multi-threading job token
    worker_pool_synctoken_jobdone(dptr->token);
}

AEEResult qhci_matmul_int8(
    remote_handle64 handle,
    const uint8_t* mIn0, //input left matrix, MxK
    int mIn0Len,
    const uint8_t* mIn1, //input right matrix, KxN
    int mIn1Len,
    uint32_t M, //mIn0 row
    uint32_t K, //mIn0 col/mIn1 row
    uint32_t N, //mIn1 col
    uint32_t* mOut, //mOut matrix MxN
    int mOutLen
)
{
    // Only supporting 128-byte aligned!!
    if (!(mIn0 && mIn1 && mOut && ((((uint32)mIn0 | (uint32)mIn1 | (uint32)mOut) & 127) == 0) \
        && (K>=4) && (K%4 == 0) && (N>=128) && (N%128 == 0)))
    {
        FARF(ERROR, "qhci_matmul_int8 input param not supported, all input need to be valid and K%4==0 and N%128==0\n");
        return AEE_EBADPARM;
    }

#ifdef PROFILING_ON
    uint64 startTime = HAP_perf_get_time_us();
    uint64 startCycles = HAP_perf_get_pcycles();
#endif
    int numWorkers = num_hvx128_contexts;

    // split src image into horizontal stripes, for multi-threading.
    worker_pool_job_t job;
    worker_synctoken_t token;

    // init the synchronization token for this dispatch.
    worker_pool_synctoken_init(&token, numWorkers);
    worker_pool_context_t *worker_pool_context = (worker_pool_context_t *)handle;
    // init job function pointer
    job.fptr = matmul_int8_callback;

    // init job data pointer. In this case, all the jobs can share the same copy of job data.
    matmul_int8_callback_t dptr;
    dptr.token = &token;
    dptr.jobCount = 0;
    dptr.mIn0 = (uint8_t*)mIn0;
    dptr.mIn1 = (uint8_t*)mIn1;
    dptr.mOut = mOut;
    dptr.M = M;
    dptr.K = K;
    dptr.N = N;
    dptr.rowsPerJob = (dptr.M + (numWorkers - 1)) / numWorkers;
    dptr.numThreads = numWorkers;
    job.dptr = (void *)&dptr;
    unsigned int i;
    for (i = 0; i < numWorkers; i++)
    {
        worker_pool_submit(*worker_pool_context, job);
    }
    worker_pool_synctoken_wait(&token);
#ifdef PROFILING_ON
    uint64 endCycles = HAP_perf_get_pcycles();
    uint64 endTime = HAP_perf_get_time_us();
    FARF(ALWAYS, "qhci_matmul_int8 profiling over %d iterations: %d PCycles, %d us. Observed clock rate %d MHz",
         LOOPS, (int)(endCycles - startCycles), (int)(endTime - startTime),
         (int)((endCycles - startCycles) / (endTime - startTime)));
#endif
    return AEE_SUCCESS;
}
