//=============================================================================
//
//  Copyright (c) 2022 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//=============================================================================

#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include "rpcmem.h"
#include "AEEStdDef.h"
#include "qhci.h"
#include "remote.h"
#define B_IN 1
#define W_IN 1920 
#define H_IN 1280
#define C_IN 3

#define B_OUT 1
#define W_OUT 1280
#define H_OUT 640
#define C_OUT 3

#ifndef CDSP_DOMAIN
#define CDSP_DOMAIN         "&_dom=cdsp"
#endif
#ifndef CDSP1_DOMAIN
#define CDSP1_DOMAIN "&_dom=cdsp1"
#endif

#ifndef CDSP_ID
#define CDSP_ID 3
#endif 

#ifndef CDSP1_ID
#define CDSP1_ID 4
#endif 
unsigned long long GetTime( void )
{
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);

	return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

static inline int set_unsigned_pd(int domain)
{
    int nErr = 0;

    if (remote_session_control)
    {
        struct remote_rpc_control_unsigned_module data;
        data.enable = 1;
        data.domain = domain;
        nErr = remote_session_control(DSPRPC_CONTROL_UNSIGNED_MODULE, (void *)&data, sizeof(data));
        printf("remote_session_control returned 0x%x for configuring unsigned PD on domain %d.\n", nErr, domain);
    }
    else
    {
        nErr = -1;
        printf("Unsigned PD not supported on this device.\n");
    }
    return nErr;
}

int main(){
	
	//if need set unsigned PD, call related function here, the default is signed PD 
	//set unsigned PD for CDSP0 here
	set_unsigned_pd(CDSP_ID);
	
	uint8_t* pSrc, *pDst,*pGolden;
	uint32_t srcSize = B_IN * W_IN * H_IN  * C_IN; //UYVY
    uint32_t dstSize = B_OUT * W_OUT * H_OUT  * C_OUT;  // BGR
	
	rpcmem_init();
    pSrc    = (uint8_t*)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, srcSize);
    pDst    = (uint8_t*)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, dstSize);
    pGolden = (uint8_t*)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, dstSize);

	if(pSrc==NULL || pDst==NULL || pGolden==NULL){
		printf("alloc failure \n");
		return  -1;
	}
    memset(pSrc, 0, srcSize);
    memset(pDst, 0, dstSize);
    memset(pGolden, 0, dstSize);
	{
		//printf("try to open data\n");
		//FILE *fdSrc     = fopen("/data/local/tmp/ruzhongl/data.bin", "rb");
		//fread(pSrc, srcSize, 1, fdSrc);
		//fclose(fdSrc);
		for(int i = 0; i < srcSize;i++)
			pSrc[i] = i%255;
	}
	

	int nErr = 0;
	uint64_t handle = -1;
    char *qhci_URI_Domain = NULL;
	qhci_URI_Domain = (char*)qhci_URI CDSP_DOMAIN; // try opening handle on CDSP0.
	nErr = qhci_open(qhci_URI_Domain, &handle);
	if(nErr){
		printf("open session failure\n");
		return -1;
	}

	int power_level = 7;
    int latency = 100;
    int dcvs_enable = 0;
    nErr = qhci_setClocks(handle, power_level, latency, dcvs_enable);
	
	int ret ;
	unsigned long long t1, t2;
	int debug_count = 0;
	while(debug_count++ < 10000){
		t1 = GetTime();
		ret = qhci_resize_near(handle,pSrc,srcSize,W_IN,H_IN, C_IN,pDst,dstSize,W_OUT,H_OUT,C_OUT);
		t2 = GetTime();
		printf("resize_near execute time: %llu us result %d \n", t2-t1,ret);
		if(ret){
			printf("resize_near error us %d %d\n", debug_count,ret);
			break;
		}
	}
	
    nErr = qhci_setClocks(handle, power_level, 0, dcvs_enable);
    //5. close handle
    (void)qhci_close(handle);
	
	return 0;
}