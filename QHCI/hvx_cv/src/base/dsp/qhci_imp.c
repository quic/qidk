//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include "HAP_farf.h"

#include <string.h>
#include <stdlib.h>

// profile DSP execution time (without RPC overhead) via HAP_perf api's.
#include "HAP_perf.h"
#include "HAP_power.h"
#include "worker_pool.h"

#include "AEEStdErr.h"

// includes
#include "qhci.h"

// HAP_mem_get_stats
#include "HAP_mem.h"

//HAP_clock_limit
#include "HAP_clock_limit_user.h"


AEEResult qhci_open(const char *uri, remote_handle64 *h)
{
    AEEResult res;
    worker_pool_context_t *worker_pool_context = (worker_pool_context_t*)malloc(sizeof(worker_pool_context_t));

    if(worker_pool_context == NULL)
    {
        FARF(ERROR, "Could not allocate memory for worker pool context");
        return AEE_ENOMEMORY;
    }
    // init worker pool
    res = worker_pool_init(worker_pool_context);
    if(res != AEE_SUCCESS)
    {
        free(worker_pool_context);
        FARF(ERROR, "Unable to create worker pool");
        return res;
    }
    // Initialize handle with worker_pool context, that way each benchmak instantiation
    // has different state so worker pool deinit will close right worker pool.
    *h = (remote_handle64)worker_pool_context;
    return AEE_SUCCESS;
}

AEEResult qhci_close(remote_handle64 h)
{
    worker_pool_context_t *worker_pool_context = (worker_pool_context_t*)h;

    if(worker_pool_context == NULL)
    {
        FARF(ERROR, "remote handle is NULL");
        return AEE_EBADHANDLE;
    }
    //deinit worker pool
    worker_pool_deinit(worker_pool_context);
    free(worker_pool_context);
    return AEE_SUCCESS;
}

AEEResult qhci_setClocks(remote_handle64 h, int32 power_level, int32 latency, int32 dcvs_enable)
{

    // Set client class (useful for monitoring concurrencies)
    HAP_power_request_t request;
    memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
    request.type = HAP_power_set_apptype;
    request.apptype = HAP_POWER_COMPUTE_CLIENT_CLASS;

    void *qhci_ctx = (void*) (h);
    int retval = HAP_power_set(qhci_ctx, &request);
    if (retval) return AEE_EFAILED;

    if(latency == 0 || latency >= 60000) {
        HAP_power_destroy_client((void*) qhci_ctx);
        return AEE_SUCCESS;
    }
    // Configure clocks & DCVS mode
    memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
    request.type = HAP_power_set_DCVS_v2;

    // Implementation detail - the dcvs_enable flag actually enables some performance-boosting DCVS features
    // beyond just the voltage corners. Hence, a better way to "disable" voltage corner DCVS when not desirable
    // is to set dcvs_enable = TRUE, and instead lock min & max voltage corners to the target voltage corner. Doing this
    // trick can sometimes get better performance at minimal power cost.
    //request.dcvs_v2.dcvs_enable = dcvs_enable;   // enable dcvs if desired, else it locks to target corner
    request.dcvs_v2.dcvs_enable = TRUE;
    request.dcvs_v2.dcvs_params.target_corner = power_level;

    if (dcvs_enable)
    {
        request.dcvs_v2.dcvs_params.min_corner = HAP_DCVS_VCORNER_DISABLE; // no minimum
        request.dcvs_v2.dcvs_params.max_corner = HAP_DCVS_VCORNER_DISABLE; // no maximum
    }
    else
    {
        request.dcvs_v2.dcvs_params.min_corner = request.dcvs_v2.dcvs_params.target_corner;  // min corner = target corner
        request.dcvs_v2.dcvs_params.max_corner = request.dcvs_v2.dcvs_params.target_corner;  // max corner = target corner
    }

    request.dcvs_v2.dcvs_option = HAP_DCVS_V2_PERFORMANCE_MODE;
    request.dcvs_v2.set_dcvs_params = TRUE;
    request.dcvs_v2.set_latency = TRUE;
    request.dcvs_v2.latency = latency;
    retval = HAP_power_set(qhci_ctx, &request);
    if (retval) return AEE_EFAILED;
// vote for HVX power
    memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
    request.type = HAP_power_set_HVX;
    request.hvx.power_up = TRUE;
    retval = HAP_power_set(qhci_ctx, &request);

    if (retval) return AEE_EFAILED;
    return AEE_SUCCESS;
}

AEEResult qhci_HAP_mem_get_stats(remote_handle64 h, uint64 *status, int statusLen)
{
    struct HAP_mem_stats *struct_status;
    struct_status = (struct HAP_mem_stats*)status;
    return HAP_mem_get_stats(struct_status);
}

AEEResult qhci_HAP_core_clock_limit_set(remote_handle64 h, uint32_t clkMHz)
{
    AEEResult limit = HAP_core_clock_limit_set(clkMHz);
    #ifdef PROFILING_ON
        if(limit == AEE_EVERSIONNOTSUPPORT) 
        {
            FARF(HIGH, "EVERSION NOT SUPPORT ");
        }
        if(limit != 0) 
        {
            FARF(HIGH, "Could not Limit clock,error:%d", limit);
        }
        else
        {
            FARF(HIGH, "Success Limit clock");
        }
    #endif
    return limit;
}

AEEResult qhci_HAP_core_clock_limit_query(remote_handle64 h, uint32_t *limit_info, int limit_infoLen)
{
    int limit_query = HAP_core_clock_limit_query((sysmon_clocklimit_info_t*)limit_info);
    #ifdef PROFILING_ON
        if(limit_query!=0)
        {
            FARF(HIGH, "clock limit query error: %d", limit_query);
        }
        else
        {
            FARF(HIGH, "++++Limit clock: %d Core clock : %d", limit_info.limit_status, limit_info.core_clock);
        }
    #endif
    return limit_query;
}
