//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include "QhciBase.hpp"
#include <iostream>

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

QhciTest::QhciTest() {
    int nErr;
    #ifdef CDSP1_ENABLE
    set_unsigned_pd(4);
    #else
    set_unsigned_pd(3);
    #endif
    rpcmem_init();

    //1. open handle
    char *qhci_URI_Domain = NULL;
    #ifdef CDSP1_ENABLE
    qhci_URI_Domain = (char*)qhci_URI CDSP1_DOMAIN; // try opening handle on CDSP1.
    #else
    qhci_URI_Domain = (char*)qhci_URI CDSP_DOMAIN; // try opening handle on CDSP0.
    #endif
    nErr = qhci_open(qhci_URI_Domain, &handle);
    CHECK(nErr==AEE_SUCCESS);

    

    //2. setclock
    int power_level = 6;
    int latency = 100;
    int dcvs_enable = 0;
    nErr = qhci_setClocks(handle, power_level, latency, dcvs_enable);
    CHECK(nErr==AEE_SUCCESS);
}

QhciTest::~QhciTest() {
    int nErr;
    //1. disable clock
    int power_level = 6;
    int latency = 0;
    int dcvs_enable = 0;
    nErr = qhci_setClocks(handle, power_level, latency, dcvs_enable);
    CHECK(nErr==AEE_SUCCESS);
    //2. close handle
    (void)qhci_close(handle);
    rpcmem_deinit();
    MapRelease();
}

int QhciTest::QhciTestAll() {
    int ret = 0;
    for (auto it = current_map_func().begin(); it != current_map_func().end(); ++it) {
        ret = QhciTest::QhciTestSingle(it->first);
        CHECK(ret==0);
    }
    return ret;
}

int QhciTest::QhciTestSingle(std::string feature) {
    int ret = 0;
    std::cout << "--------------------------------------" << std::endl;
    std::cout << "run unit test: " << feature << " start" << std::endl;
    ret = current_map_func()[feature]->Test(handle);
    std::cout << "run unit test: " << feature << " end" << std::endl;
    return ret;
}

void QhciTest::MapRelease(){
    for (auto& element : current_map_func()) {
        delete element.second;
    }
}

void QhciTest::PringUsage(void) {
    std::string usage;
    usage += "Usage ./qhci_test all\n";
    for (auto it = current_map_func().begin(); it != current_map_func().end(); ++it) {
        usage += "                  " + it->first + "\n";
    }
    std::cout << usage << std::endl;
}

int QhciFeatureBase::Test(remote_handle64 handle){
    int ret = 0;
    std::cout << "Test feature: " << name  << " Fastrpc handle: " << handle << std::endl;
    return ret;
}

