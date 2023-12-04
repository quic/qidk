# QHCI
QHCI (Qualcomm HTP Customized Interface)

QHCI is a repo includes QNN UDO and customized CV algos based on HVX which are implemented by CE Team.

The repo have Qcom license and public to all customers in the form of library release.

## Directory Structure

- Main dir
    - doc:  docs for features HVX implemention introduce
    - qhci: main project dir include all sources, only for internal
    - release: release libs, header, sampleApp, public to all customers.
```
QHCI$ tree -L 1
.
├── doc
├── qhci
├── README.md
└── release

```
- qhci sub dir
    - src_app: include all AP side code
    - src_dsp: include all dsp side code
    - asm_src: include ASM code
    - sampleApp: A simple sample APP with cmake, to introduce how to integrate qhci lib.
    - testcase: each feature testcases
    - cpu.min: When add a new feature, need add related AP src to this makefile
    - hexagon.min: When add a new feature, need add related DSP src to this makefile.

```
QHCI/qhci$ tree -L 1
.
├── android_deps.min
├── android.min
├── asm_src
├── build_AP.bat
├── build_hvx.bat
├── build_qnx.bat
├── cpu.min
├── hexagon_deps.min
├── hexagon.min
├── inc
├── libworker_pool.a
├── Makefile
├── qnx_deps.min
├── qnx.min
├── sampleApp
├── src_app
├── src_dsp
└── testcase

```
- src_app
    - inc:  include CPU reference&test code header
    - uyvy2bgr/... : each feature have separately AP src dir
    - qhci.c: main testbed code, calling to all features testbed
    - uyvy2bgr_ref.c/... : each feature reference CPU implementaion
    - uyvy2bgr_test.c/... : each feature testbed code

```
QHCI/qhci/src_app$ tree -L 2
.
├── inc
│   ├── qhci_ref.h
│   └── qhci_test.h
├── qhci.c
├── resize_near
│   └── resize_near_test.c
├── rgb2yuv
│   ├── rgb2yuv_ref.c
│   ├── rgb2yuv_test.c
│   └── yuv2rgb_ref.c
├── uyvy2bgr
│   ├── uyvy2bgr_ref.c
│   └── uyvy2bgr_test.c
├── fft_for_xiaomi_16x16
│   ├── fft_for_xiaomi_16x16_ref.c
│   └── fft_for_xiaomi_16x16_test.c
├── ifft_for_xiaomi_32x32
│   ├── ifft_for_xiaomi_32x32_ref.c
│   └── ifft_for_xiaomi_32x32_test.c
├── ifft_for_xiaomi_64x64
│   ├── ifft_for_xiaomi_64x64_ref.c
│   └── ifft_for_xiaomi_64x64_test.c
├── gaussian5x5_uint16
│   ├── gaussian5x5_uint16_ref.c
│   └── gaussian5x5_uint16_test.c
├── div16_integer_test_Denome0_result0
│   ├── div16_integer_test_Denome0_result0_ref.c
│   └── div16_integer_test_Denome0_result0_test.c
├── div16_integer_test_signed
│   ├── div16_integer_test_signed_ref.c
│   └── div16_integer_test_signed_test.c
├── div16_fractional_test_integer
│   ├── div16_fractional_test_integer_ref.c
│   └── div16_fractional_test_integer_test.c
├── ccclrGrd_opt
│   ├── ccclrGrd_opt_ref.c
│   └── ccclrGrd_opt_test.c
└── sInt32Division
    ├── sInt32Division_ref.c
    └── sInt32Division_test.c

```
- src_dsp
    - inc:  include DSP code header
    - uyvy2bgr/... : each feature have separately DSP src dir
    - qhci_imp.c: main DSP code, open/close handle, set clock, etc.
    - uyvy2bgr_imp.c/... : each feature DSP HVX implementaion
```
QHCI/qhci/src_dsp$ tree -L 2
.
├── inc
│   ├── q6cache.h
│   ├── verify.h
│   ├── RIFFT16x16_4_asm.h
│   └── vmemcpy.h
├── qhci_imp.c
├── resize_near
│   └── resize_near_impl.c
├── rgb2yuv
│   ├── rgb2yuv_imp.c
│   └── yuv2rbg_imp.c
├── uyvy2bgr
│   └── uyvy2bgr_imp.c
├── fft_for_xiaomi_16x16
│   └── fft_for_xiaomi_16x16_imp.c
├── ifft_for_xiaomi_32x32
│   ├── ifft_for_xiaomi_32x32_imp.c
│   ├── FFT32x32_C_intrinsics.c
│   └── IFFT32x32_C_intrinsics.c
├── ifft_for_xiaomi_64x64
│   ├── ifft_for_xiaomi_64x64_imp.c
│   ├── FFT64x64_C_intrinsics.c
│   └── IFFT64x64_C_intrinsics.c
├── gaussian5x5_uint16
│   ├── gaussian5x5_uint16_imp.c
│   └── gaussian5x5_C_intrinsics.c
├── div16_integer_test_Denome0_result0
│   └── div16_integer_test_Denome0_result0_imp.c
├── div16_integer_test_signed
│   └── div16_integer_test_signed_imp.c
├── div16_fractional_test_integer
│   └── div16_fractional_test_integer_imp.c
├── ccclrGrd_opt
│   ├── ccclrGrd_opt_imp.c
│   └── ccclrGrd_C_intrinsics.c
└── sInt32Division
    └── sInt32Division_imp.c
```

## Rule
- API Name Define: qhci_xxx()
    - All API define followed by "qhci_"
    - eg. qhci_open, qhci_close

## How to intgerate a new feature ?
- Create a xxx feature sub dir under src_app and src_dsp
- Add API define in QHCI/qhci/inc/qhci.idl
- Implement feature reference CPU code
- Implement feature DSP HVX code
- Implement feature CPU testbed code
- Add reference&test API define to src_app/inc/qhci_ref.h and src_app/inc/qhci_test.h
- Add CPU code relationship to cpu.min
```
$(EXE_NAME)_C_SRCS += src_app/uyvy2bgr/uyvy2bgr_ref \
                    src_app/uyvy2bgr/uyvy2bgr_test \
```
- Add DSP code relationship to hexagon.min
```
libqhci_skel_C_SRCS += $(OBJ_DIR)/qhci_skel \
                            src_dsp/qhci_imp \
```
- Add feature testbed to main QHCI/qhci/src_app/qhci.c
```
static int parse_config(int argc, char *argv[], qhci_config_t *cfg) {
    ...
        switch (opt) {
            case 'f':
                if (!strncmp(optarg, "uyvy2bgr", strlen("uyvy2bgr"))) {
                    cfg->idx = UYVY2BGR;
                    ret = 0;
                } else if (!strncmp(optarg, "rgb2yuv", strlen("rgb2yuv"))) {
                    cfg->idx = RGB2YUV;
                    ret = 0;
                ...

int test_main_start(int argc, char *argv[])
{
    ...
    switch(params.idx) {
        case UYVY2BGR:
            ret = uyvy2bgr_test();
            break;
        case RGB2YUV:
            ret = rgb2yuv_test();
            break;
        ...
```
- Create testbed dir under QHCI/qhci/testcase, add related input&script
```
QHCI/qhci/testcase/uyvy2bgr$ tree -L 1
.
├── input.uyvy
├── run_qnx.sh
└── run.sh
```
- Recompile project to generate android/qnx/dsp libs
- Update new libs in QHCI/release/libs
```
QHCI/release/libs$ tree -L 2
.
├── aarch64-android
│   └── libqhci.so
├── aarch64-qnx
│   └── libqhci.so
└── hexagon-v68
    ├── libqhci_skel.a
    └── libqhci_skel.so
```
- Update new feature API in header release/API/qhci.h, manual add API/param introduce.
```
//------------------------------------------------------------------------------
/// @brief
///   close a exist fastrpc handle
/// @param _h
///   fastrpc handle, created by qhci_open
/// @return
///   0 upon success(AEE_SUCCESS).
///   Other status codes upon failure.
int qhci_close(uint64_t h);

```

