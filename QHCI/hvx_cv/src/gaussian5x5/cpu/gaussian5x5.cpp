#include "QhciBase.hpp"
#define FEATURE_NAME "gaussian5x5"

static const int GAUSS_5x5[5 * 5] = {
    1, 6, 15, 6, 1,
    6, 36, 90, 36, 6,
    15, 90, 225, 90, 15,
    6, 36, 90, 36, 6,
    1, 6, 15, 6, 1};
// Step0. Define reference code here
AEEResult qhci_gaussian5x5_ref(
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
    
    int x, y, s, t;
    int sum, out;
    for (int loops = 0; loops < LOOPS; loops++)
    {
    for (y = 2; y < srcHeight - 2; y++)
    {
        for (x = 2; x < srcWidth - 2; x++)
        {
            sum = 0;

            for (s = -2; s <= 2; s++)
            {
                for (t = -2; t <= 2; t++)
                {

                    sum += src[(y + t) * srcStride + x + s] * GAUSS_5x5[((t + 2) * 5) + (s + 2)];
                }
            }

            out = sum >> 12;

            out = out < 0 ? 0 : out > 65535 ? 65535
                                            : out;

            dst[y * dstStride + x] = (unsigned short)out;
        }
    }
    }
    return AEE_SUCCESS;
}

class GAUSSIAN5X5 : public QhciFeatureBase
{
public:
    using QhciFeatureBase::QhciFeatureBase;
    int Test(remote_handle64 handle) override
    {
        AEEResult nErr = 0;
        // Api teat
        {
            // Step1. Define param && allocate buffer
            uint32_t srcLen = 1920 * 1080;
            uint16_t *src = (uint16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, srcLen * sizeof(uint16_t));
            uint32_t srcWidth = 1920;
            uint32_t srcHeight = 1080;
            uint32_t srcStride = 1920;
            uint32_t dstStride = 1920;
            uint32_t dstLen = 1920 * 1080;
            uint16_t *dst_cpu = (uint16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, dstLen * sizeof(uint16_t));
            uint16_t *dst_dsp = (uint16_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, dstLen * sizeof(uint16_t));

            // Step2. Random generate input data, if need to use real data, pls comment this and implement by yourself.
            GenerateRandomData((uint8_t *)src, srcLen * sizeof(uint16_t));

            // Step3. CPU execute
            TIME_STAMP("Cpu", qhci_gaussian5x5_ref(handle, src, srcLen, srcWidth, srcHeight, srcStride, dstStride, dst_cpu, dstLen));

            // Step4. DSP execute
            TIME_STAMP("DSP", qhci_gaussian5x5(handle, src, srcLen, srcWidth, srcHeight, srcStride, dstStride, dst_dsp, dstLen));

            // Step5. Compare CPU & DSP results
            int bitexactErrors = 0;
            for (int j = 2; j < srcHeight - 2; j++)
            {
                for (int i = 2; i < srcWidth - 2; i++)
                {
                    if (dst_cpu[j * dstStride + i] != dst_dsp[j * dstStride + i])
                    {
                        bitexactErrors++;
                        if (bitexactErrors < 10)
                            printf("Bit exact error: j=%d, i=%d, dst_cpu=%d, dst_dsp=%d\n", j, i, dst_cpu[j * dstStride + i], dst_dsp[j * dstStride + i]);
                    }
                }
            }
            if (!bitexactErrors)
            {
                std::cout << "Verify pass! DSP results same with CPU." << std::endl;
            }
            CHECK(0 == bitexactErrors);

            // Step6. Free buffer
            if (src)
                rpcmem_free(src);
            if (dst_cpu)
                rpcmem_free(dst_cpu);
            if (dst_dsp)
                rpcmem_free(dst_dsp);
        }

        return nErr;
    }
};

static GAUSSIAN5X5 *_feature = new GAUSSIAN5X5(FEATURE_NAME);
static bool init = []
{
    QhciTest::GetInstance().current_map_func()[FEATURE_NAME] = _feature;
    return true;
}();
#undef FEATURE_NAME
