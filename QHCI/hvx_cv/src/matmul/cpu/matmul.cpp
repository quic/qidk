#include "QhciBase.hpp"

#define FEATURE_NAME "matmul"

//Step0. Define reference code here
AEEResult qhci_matmul_int8_ref(
    remote_handle64 handle,
    const uint8_t* mIn0, //input left matrix, MxK
    int mIn0Len,
    const uint8_t* mIn1, //input right matrix, KxN
    int mIn1Len,
    uint32_t M, //in0 row
    uint32_t K, //in0 col/in1 row
    uint32_t N, //in1 col
    uint32_t* mOut, //out matrix MxN
    int mOutLen
)
{
    memset(mOut, 0, sizeof(uint32_t)*M*N);
    for (int m=0; m<M; m++) {
        for (int n=0; n<N; n++) {
            for (int k=0; k<K; k++) {
                mOut[m*N+n] += (uint32_t)(mIn0[m*K+k]) * (uint32_t)(mIn1[k*N+n]);
            }
        }
    }
    return AEE_SUCCESS;
}


class MATMUL : public QhciFeatureBase {
public:
    using QhciFeatureBase::QhciFeatureBase;
    int Test(remote_handle64 handle) override {
        AEEResult nErr = 0;
        //Api teat
        {
            //Step1. Define param && allocate buffer
            uint32_t mIn0Len = 133*512;
            uint8_t* mIn0 = (uint8_t*)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, mIn0Len*sizeof(uint8_t));
            uint32_t mIn1Len = 512*2048;
            uint8_t* mIn1 = (uint8_t*)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, mIn1Len*sizeof(uint8_t));
            uint32_t M = 133;
            uint32_t K = 512;
            uint32_t N = 2048;
            uint32_t mOutLen = 133*2048;
            uint32_t* mOut_cpu = (uint32_t*)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, mOutLen*sizeof(uint32_t));
            uint32_t* mOut_dsp = (uint32_t*)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, mOutLen*sizeof(uint32_t));

            //Step2. Random generate input data, if need to use real data, pls comment this and implement by yourself.
            GenerateRandomData((uint8_t*)mIn0, mIn0Len*sizeof(uint8_t));
            GenerateRandomData((uint8_t*)mIn1, mIn1Len*sizeof(uint8_t));

            //Step3. CPU execute
            TIME_STAMP("Cpu", qhci_matmul_int8_ref(handle,mIn0,mIn0Len,mIn1,mIn1Len,M,K,N,mOut_cpu,mOutLen));

            //Step4. DSP execute
            TIME_STAMP("DSP", qhci_matmul_int8(handle,mIn0,mIn0Len,mIn1,mIn1Len,M,K,N,mOut_dsp,mOutLen));

            //Step5. Compare CPU & DSP results
            nErr = CompareBuffers(mOut_cpu, mOut_dsp, mOutLen);
            CHECK(nErr==AEE_SUCCESS);

            //Step6. Free buffer
            if(mIn0) rpcmem_free(mIn0);
            if(mIn1) rpcmem_free(mIn1);
            if(mOut_cpu) rpcmem_free(mOut_cpu);
            if(mOut_dsp) rpcmem_free(mOut_dsp);
        }

        return nErr;
    }
};

static MATMUL* _feature = new MATMUL(FEATURE_NAME);
static bool init = [] {
    QhciTest::GetInstance().current_map_func()[FEATURE_NAME] = _feature;
    return true;
}();
#undef FEATURE_NAME
