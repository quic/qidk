# Introduction : UDO Implementation for Un-supported op SELU

Qualcomm® Neural Processing SDK offers a wide range of operations/layers from different ML frameworks like Caffe, ONNX, PyTorch and TensorFlow. For the operations which are not supported by the Qualcomm® Neural Processing SDK, UDO feature is provided to users.
UDO stands for User Defined Operations. UDO allows users to implement the layer/operation defined in popular frameworks but not yet supported by Qualcomm® Neural Processing SDK.

# Pre-requisites

* Required QNN-2.6.x
	* Download QNN-2.6.x, un-zip the package and 
	* Run ```export QNN_SDK_ROOT=/complete/path/till/qnn-v2.6.x```

* Required SNPE-2.6.x
	* Qualcomm® Neural Processing SDK for AI setup should be completed by following the guide here: https://developer.qualcomm.com/sites/default/files/docs/snpe/setup.html
* python version 3.6
* TensorFlow version 2.6
* Install ```pip install jupyter```.
* Netron only if interested in Visualizing the TensorFlow frozen graph (Optional)
* Ubuntu 18.04.6 LTS
* Device used: Snapdragon® SM8550
* Clone this git repo and cd to the offline repo

# Steps to create UDO
The SELU UDO implementation provided in this git repo is for CPU runtime only.
 
## Defining a UDO Package

* Configuration for the operation needs to be put in a config file. Below, is an example of such a config file. <br>
<I>The config file used for this experiment is different and can be found in this repo by the name Selu.json </I>
```
{
    "UdoPackage_0":
    {
        "Operators": [
            {
                "type": "",
                "inputs":[
                    {"name":"", "per_core_data_types":{"CPU":"FLOAT_32", "GPU":"FLOAT_32", "DSP":"UINT_8"},
                    "static": true, "tensor_layout": "NHWC"},
                    {"name":"", "data_type": "FLOAT_32",
                    "static": true, "tensor_layout": "NHWC"},
                ],
                "outputs":[
                    {"name":"", "per_core_data_types":{"CPU":"FLOAT_32", "GPU":"FLOAT_32", "DSP":"UINT_8"}}
                    {"name":"", "data_type": "FLOAT_32"}
                ],
                "scalar_params": [
                    {"name":"scalar_param_1", "data_type": "INT_32"}
                ],
                "tensor_params": [
                    {"name":"tensor_param_1", "data_type": "FLOAT_32", "tensor_layout": "NHWC"},
                ],
                "core_types": ["CPU", "GPU", "DSP"],
                "dsp_arch_types": ["v66", "v68"]
            }
        ],
        "UDO_PACKAGE_NAME": "MyCustomUdoPackage",
    }
}

``` 

Basically, in this config file the fields are specified in key-value pairs.
* UdoPackage: Every UDO package can be described as "UdoPackage_i" where i indicates the order in which the packages will be generated. The user is also free to use empty strings, but the dictionary structure is necessary.
* Operators: This is a child node of UdoPackage indicating the number of operators present.
	* type: defines the type of the operation.
	* inputs: a list of input tensors and their data_types to the operation.
		* name: An optional field that describes the name of the input tensor. Since the name of the input tensor is variable, the user is not required to provide this.
		* per_core_data_type: A dictionary object specifying the data-type of this input tensor in each core. Alternatively, if the user wishes to have the same data-type across all specified cores, then the user can specify the option "data_type" followed by the data-type (ref. Selu.json). The supported data-types are FLOAT_16, FLOAT_32, FIXED_4, FIXED_8, FIXED_16, UINT_8, UINT_16 and UINT_32. 
		* static: A boolean field that is required if the input data is static i.e. data is provided in the model. This field needs to be set if the input tensor will contain data, otherwise the input will be treated dynamically, and the data will not be serialized.
		* tensor_layout: A string field that describes the canonical dimension format of the input tensor. The supported values are NCHW and NHWC.
	* outputs: A list of output tensors to the operation.
	* scalar_params: A list of scalar-valued attributes.1
		* name: A required field that describes the name of the scalar parameter.
		* data_type: A required field that describes the data-type supported by this scalar parameter.
	* tensor_params: A list of tensor-valued attributes.2 3
	* core_types: The intended IP cores for this particular operation. The supported core_types CPU, GPU and DSP.
	* dsp_arch_types: The intended DSP architecture types for DSP core type. The supported dsp_arch_types v65, v66 and v68.
* UDO_PACKAGE_NAME: The name of the UDO Package, which can be any valid string.

## Creating a UDO Package

* Once a configuration has been specified to adequately represent the desired UDO, it can be supplied as an argument to the SNPE UDO package generator tool "snpe-udo-package-generator". The intention of the tool is to generate partial skeleton code to aid rapid prototyping. 
* Run the below command to create UDO package<br>
```python 
snpe-udo-package-generator -p config/Selu.json -o .
```
* "SeluUdoPackage" can now be seen in base folder. 
* Below section shows the artifacts it generates. 
```python
SeluUdoPackage/
├── common.mk
├── config
│   └── Selu.json
├── include
│   ├── SeluUdoPackageCpuImplValidationFunctions.hpp
│   └── utils
│       ├── IUdoOpDefinition.hpp
│       ├── UdoCpuOperation.hpp
│       ├── UdoDspShared.h
│       ├── UdoMacros.hpp
│       ├── UdoOperation.hpp
│       └── UdoUtil.hpp
├── jni
│   ├── Android.mk
│   ├── Application.mk
│   └── src
│       ├── CPU
│       │   ├── Makefile
│       │   ├── makefiles
│       │   │   ├── Android.mk
│       │   │   ├── Application.mk
│       │   │   └── Makefile.linux-x86_64
│       │   └── src
│       │       ├── ops
│       │       │   └── Selu.cpp
│       │       ├── SeluUdoPackageInterface.cpp
│       │       └── utils
│       │           ├── BackendUtils.hpp
│       │           ├── CPU
│       │           │   ├── CpuBackendUtils.cpp
│       │           │   └── CpuBackendUtils.hpp
│       │           └── CustomOpUtils.hpp
│       ├── reg
│       │   ├── Makefile
│       │   ├── SeluUdoPackageCpuImplValidationFunctions.cpp
│       │   └── SeluUdoPackageRegLib.cpp
│       └── utils
│           ├── UdoCpuOperation.cpp
│           └── UdoUtil.cpp
└── Makefile
```

## Completing the Implementation Skeleton Code

For this example, user can copy the selu implementation already provided by running the below command
```python 
cp selu_cpp_implementation/Selu.cpp SeluUdoPackage/jni/src/CPU/src/ops
```

File "Selu.cpp" contains the implementation w.r.t. SELU operation.<br><br>
<I>Otherwise, for creating a UDO on their own user need to write their UDO function in Selu.cpp which is present on the path “SeluUdoPackage/jni/src/CPU/src/ops/Selu.cpp”.</I>

## Compiling a UDO package

* Change directory to "SeluUdoPackage".
```python
cd SeluUdoPackage/
```
* Now run "make" command in cmd
```python
make
```
* Once the "make" command has run successfully user can check SeluUdoPackage/libs for library folders for different architecture like 
	* <b>x86-64_linux_clang :</b> Libraries from this folder will be used to run the UDO from linux host machine
	* <b>arm64-v8a :</b> Libraries from this folder will be used to run the UDO from device (like Snapdragon® SM8550)

# Preparing Demo Model to Run Using SELU UDO Implementation

## Model Conversion to DLC format
* DLC stands for Deep Learning Container. It is a format which is supported by Qualcomm® hardwares to run machine learning models. Here, we are converting the TensorFlow ".pb" model file to ".dlc".
* TensorFlow model frozen graph "*.pb" file is shared in this git repo.
* Run below command to do model conversion to dlc using snpe-tensorflow-to-dlc. <br>
```python
cd ..
snpe-tensorflow-to-dlc --input_network model/frozen_graph.pb --input_dim 'x' 1,28,28,1 --out_node Identity --output_path model/selu_udo.dlc --udo config/Selu.json
```

## Executing model on x86 Host using snpe-net-run
* Expecting user to be in "UDO_SELU" base directory.
* Set the binaries path for x86 machine
```python
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/SeluUdoPackage/libs/x86-64_linux_clang
```
* Running the model on x86 host machine.
```python
snpe-net-run --container model/selu_udo.dlc --input_list raw_list.txt --udo_package_path SeluUdoPackage/libs/x86-64_linux_clang/libUdoSeluUdoPackageReg.so
```
* An "output" folder can be seen after executing the model, as mentioned above.
 
## Android CPU Execution
* Target device should be connected to the host machine.
* Run below command to run jupyter notebook. 
```python
jupyter notebook --no-browser --port=8080 --ip 0.0.0.0 --allow-root
```
* After, running the command user will get a url link. Copy paste the url link which has host machine name or "localhost" in it, into any browser. Navigate to scripts folder and open the "*.ipynb" file. Run the code, block by block.

* The code should end with a **"Successfully executed!"** message, after executing the model on target.
