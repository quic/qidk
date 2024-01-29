# SNPE-Helper

SNPE Helper is a Python API wrapper for C++ API provided by [Qualcomm Neural Processing SDK](https://developer.qualcomm.com/sites/default/files/docs/snpe/overview.html). 

SNPE-Helper is verified for Windows-on-Snapdragon, and LNX.LE builds. 

1. snpehelper for WoS - Current ReadMe File. 
2. snpehelper for LNX.LE - [ReadMe](./README_LE_1_0.md)

Tools and APIs for Execution and Easier Integration of your DNN project.
<br>

#### >> [Tutorials](Tutorials/), [How to use](Tutorials/README.md)

## Index:
- [Requirements / Dependencies](#requirements--dependencies)
- [How to install this package](#how-to-install-this-package)
- [How to Uninstall](#how-to-uninstall)
- [Overview](#overview)
- [SnpeContext Class](#snpecontext-class)
- [Project INFO](#project-info)

## Requirements / Dependencies:
```
pip install setuptools
pip install prebuilt_binaries
pip install pybind11
```
- Download and install CMAKE tool for Windows ARM64
- [SNPE SDK](https://developer.qualcomm.com/software/qualcomm-neural-processing-sdk/tools) for the respective Snapdragon platforms and a good Python IDE (like PyCharm/VS-Code/...)

## How to install this package:
#### NOTE: Make sure to set the correct path in snpehelper/CMakeLists.txt
```
set (SNPE_ROOT "C:/Qualcomm/AIStack/SNPE/2.15.1.230926")
set (PYTHON_INCLUDE_DIR "C:/Users/HCKTest/AppData/Local/Programs/Python/Python38/include")
set (PYTHON_LIB_DIR "C:/Users/HCKTest/AppData/Local/Programs/Python/Python38/libs")
set (PYBIND11_INCLUDE_DIR "C:/Users/HCKTest/AppData/Local/Programs/Python/Python38/lib/site-packages/pybind11/include")
```
A. Fresh Install:
```
git clone https://github.qualcomm.com/aicatalog/snpehelper.git (TODO)
cd snpehelper && pip install .
```

B. Upgrade / Reinstall existing package, go to `snpehelper` root dir and run:
```
git pull
pip install --upgrade .
```
C. To get debug logs enable "DEBUG" in [Utils.h](snpehelper/Utils.h#L22)
## How to Uninstall:
```
pip uninstall snpehelper
```

## Overview:

![snpehelper](https://github.qualcomm.com/storage/user/30177/files/82e8cecb-fae4-49ec-ae70-4a08237efda4)

The goal of snpehelper is to help users inference DLC models on Snapdragon Devices with a Python Interface.

## SnpeContext Class:
- `Initilize`: For allocating buffers based on the input parameters
- `SetInputBuffer`: To update input buffer with preprocessed data for specified input layer
- `Execute`: To execute DLC on Snapdragon device 
- `GetOutputBuffer`: Returns inferenced data from specified output nodes

![snpecontext](https://github.qualcomm.com/storage/user/30177/files/1d456ecd-eb0f-4aa0-b0a7-6d2f6d8d0d4b)


<br>

# NOTE: Place all DLLs and .so files in [Tutorials](Tutorials/) directory from below path
#### DLL path : "C:\Qualcomm\AIStack\SNPE\2.15.1.230926\lib\arm64x-windows-msvc"
#### so path : "C:\Qualcomm\AIStack\SNPE\2.15.1.230926\lib\hexagon-v73\unsigned"
## Project INFO:

__version__ = "0.1.0" <br>
__author__ = 'Sumith Kumar Budha' <br>
__credits__ = 'QIDK Solutions, Qualcomm' <br>

###### *Qualcomm® Neural Processing SDK is a product of Qualcomm Technologies, Inc. and/or its subsidiaries.*
