	/*-----------------------------------------------------------------------
	   Copyright (c) 2022 QUALCOMM Technologies, Incorporated.
	   All Rights Reserved.
	   QUALCOMM Proprietary.
	-----------------------------------------------------------------------*/
	 
	/**
	 *  @file HAP_clock_limit_user.h
	 *  @brief Header file with APIs to set and get user clock limit
	 */
	 
	#include "AEEStdErr.h"
	 
	#ifdef __cplusplus
	extern "C" {
	#endif
	 
	/** @defgroup Types Data types
	 *  @{
	 */
	/**
	 * Input parameter type used when user queries the clock limit
	 * status using HAP_core_clock_limit_query API.
	 */
	typedef struct
	{
	    unsigned int limit_status;
	    /**< Current clock limit vote in MHz, 0 if no limit */
	    unsigned int core_clock;
	    /**<  Current DSP core clock vote in MHz*/
	} sysmon_clocklimit_info_t;
	 
	/** @}
	 */
	 
	 /**
	 * @cond DEV
	 */
	 
	int __attribute__((weak)) __HAP_core_clock_limit_set(unsigned int clkMHz);
	int __attribute__((weak)) __HAP_core_clock_limit_query(sysmon_clocklimit_info_t *limit_info);
	 
	/**
	 * @endcond
	 */
	 
	 
	/** @defgroup helperapi Helper APIs to set/get clock limit.
	 *  API for users to set CDSP Q6 upper clock limit.
	 *  The HAP API provides user capability to set/get CDSP Q6
	 *  clock limit from a signed user process. Setting a limit will
	 *  overwrite any previous such request from the calling
	 *  process. In case of requests from multiple processes (not
	 *  suggested), the minimum among non-zero limit requests is
	 *  applied as clock limit. This API should be used cautiously
	 *  as it can lead to performance degradation when
	 *  aggressively limited. A limit set can be removed by
	 *  explicitly voting for 0. A limit set will be valid only till the
	 *  calling process is active.
	 *  @{
	 */
	 
	/**
	 * Requests clock limit
	 *
	 * Call this function from the DSP user process to set CDSP clock limit.
	 * Supported on CDSP starting with Kailua.
	 * @param Clock limit in MHz
	 * @return 0 upon success, other values upon failure.
	 */
	static inline int HAP_core_clock_limit_set(unsigned int clkMHz) {
	    if(__HAP_core_clock_limit_set)
	        return __HAP_core_clock_limit_set(clkMHz);
	    return AEE_EVERSIONNOTSUPPORT;
	}
	 
	/**
	 * Returns current clock limit stats : active limit request and
	 * current core clock in MHz.
	 * Call this function from the DSP user process to get limit request stats.
	 * active limit request will be 0 if no clock limit set.
	 * Supported on CDSP starting with Kailua.
	 * @param pointer to limit request stats structure
	 * @return 0 for success, other values on failure.
	 */
	static inline int HAP_core_clock_limit_query(sysmon_clocklimit_info_t *limit_info) {
	    if(__HAP_core_clock_limit_query)
	        return __HAP_core_clock_limit_query(limit_info);
	    return AEE_EVERSIONNOTSUPPORT;
	}
	 
	/**
	 * @}
	 */
	 
	#ifdef __cplusplus
	}
	#endif