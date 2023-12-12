# QHCI
QHCI (Qualcomm HTP Customized Interface)

QHCI is a repo includes QNN UDO and customized CV algos based on HVX which are implemented by CE Team.

The repo have Qcom license and public to all customers in the form of library release.

## Directory Structure

- Main dir
    - doc:  docs for features HVX implemention introduce
    - hvx_cv: main project dir include all hvx cv customized implementaion.
    - qnn_udo: main project dir include all qnn udo implementaion.

```
QHCI$ tree -L 1
.
├── doc
├── hvx_cv
├── qnn_udo
└── README.md

```
- hvx_cv sub dir
    - src: include all feature AP code and dsp code
    - sampleApp: A simple sample APP with cmake, to introduce how to integrate qhci lib.
    - cpu.min: When add a new feature, need add related AP src to this makefile
    - hexagon.min: When add a new feature, need add related DSP src to this makefile.
    - qhci_api_generator.py: this script can help to auto create new dir and add new dummy files for new feature.

```
QHCI/qhci/hvx_cv$ tree -L 1
.
├── android_deps.min
├── android.min
├── build_android.sh
├── build_hexagon.sh
├── cpu.min
├── flash.sh
├── hexagon_deps.min
├── hexagon.min
├── inc
├── include
├── libworker_pool.a
├── Makefile
├── qhci_api_generator.py
├── release
├── sampleApp
└── src

```
- src
    - src dir:  each feature have separately src dir for both cpu/dsp and xml to define the feature API.

```
QHCI/qhci/hvx_cv/src$ tree -L 2
.
├── base
│   ├── cpu
│   └── dsp
├── div16
│   ├── cpu
│   ├── dsp
│   └── QhciApi.xml
├── dummy
│   ├── cpu
│   └── dsp
├── FFT16_16
│   ├── cpu
│   ├── dsp
│   └── QhciApi.xml
├── gaussian5x5
│   ├── cpu
│   ├── dsp
│   └── QhciApi.xml
├── main.cpp
└── rgb2yuv
    ├── cpu
    ├── dsp
    └── QhciApi.xml

```
- qnn_udo sub dir
    - each UDO have separately src dir.
    - each sub UDO dir will include dummy model/inputs, UDO code, test scripts.

```
QHCI/qhci/qnn_udo$ tree -L 1
.
├── InputPack
└── RandomNormalLike

```
## Rule
- API Name Define: qhci_xxx()
    - All API define followed by "qhci_"
    - eg. qhci_open, qhci_close

