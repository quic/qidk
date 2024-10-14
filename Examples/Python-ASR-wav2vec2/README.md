# OnDevice Automatic Speech Recognition

- [Introduction](#introduction)
- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Model Selection and DLC conversion](#1-model-preparation)
  1. Model Overview
  2. Steps to convert model to DLC
 
- [Build and Run with Android Studio](#4-build-and-run-with-android-studio)
- [Qualcomm® Neural Processing SDK C++ APIs JNI Integration](#qualcomm-neural-processing-sdk-c-apis-jni-integration)
- [Demo](#Result)
- [Credits](#credits)
- [References](#references)

# Introduction

Automatic Speech Recognition (ASR) is one of the common and challenging Natural Language Processing tasks. <br>
- Current project is an sample Android application for OnDevice Automatic Speech Recognition using [Qualcomm® Neural Processing SDK for AI](https://developer.qualcomm.com/sites/default/files/docs/snpe/index.html) framework. 
-  We have used 1 Model in this Solution [Wav2Vec2 Model](https://huggingface.co/docs/transformers/model_doc/wav2vec2)

- We Need to Give 1 Inputs: input_values of fixed Size(1,40000)
- Models are small, efficient and mobile friendly Transformer model fine-tuned on [librispeech dataset](https://www.tensorflow.org/datasets/catalog/librispeech) for **ASR** downstream task
- In this project, we'll show how to efficiently convert, deploy and acclerate of these model on Snapdragon® platforms to perform Ondevice Automatic Speech Recognition(ASR).

## Model Architecture
<p align="center">
<img src="readme-assets/wav2vec2_architecture.png" width=35% height=35%>
</p>


## Prerequisites
* Android Studio to import and build the project
* Android NDK "r19c" or "r21e" to build native code in Android Studio
* Python 3.8, PyTorch 1.10.1, Tensorflow 2.6.2, Transformers 4.18.0, Datasets 2.4.0 to prepare and validate the model<br>
  ###### <i>(above mentioned Python packages version and Android Studio version is just a recommendation and is not a hard requirement. Please install SDK dependencies in Python 3.8 virtual environment) </i>
* [Qualcomm® Neural Processing Engine for AI SDK](https://developer.qualcomm.com/software/qualcomm-neural-processing-sdk) v2.x.x and its [dependencies](https://developer.qualcomm.com/sites/default/files/docs/snpe/setup.html) to integrate and accelerate the network on Snapdragon<br>
  ###### <i>(During developement of this tutorial, the AI SDK recommends Python 3.8 version and is subject to change with future SDK releases. Please refer SDK Release Notes.)</i>
  

## Quick Start

### 1. Model Preparation
Please go to wav2vec2_model_generation/accuracy_analyzer.ipynb file to generate the models and put the models inside **app/src/main/assets** folder,**Before running this Notebook Setup your SNPE Environment from the below code.**
<br>
#### 1.2 Setup the Qualcomm® Neural Processing SDK Environment:
```
source <snpe-sdk-location>/bin/envsetup.sh 
```


#### 1.3 Convert generated frozen graph into DLC (Deep Learning Container):
```
snpe-onnx-to-dlc -i model.onnx -d input_values 1,40000 -o wav2vec2_fp32.dlc 
```


###### <i>(you can add outputTensor name in the above command, please check SNPE Document for more information. Please check once by visualizing graph using Netron viewer or any other visualization tools )</i> <br>


#### 1.4 Quantization with caching of DLC (for optimizing model loading time on DSP accelerator)
```
snpe-dlc-quantize --input_dlc models/wav2vec2_fp32.dlc --input_list list.txt  --optimizations cle --output_dlc models/wav2vec2_w8a16.dlc --enable_htp --htp_socs sm8650 --weights_bitwidth 8 --act_bitwidth 16 
```

 <br>

- **Here In this Android App We've used wav2vec2_fp32.dlc**
- You can change for desired DLC from **app/src/main/cpp/native_lib.cpp** folder


### 4. Build and run with Android Studio

#### Add AI SDK libs and generated DLC into app assets, jniLibs and cmakeLibs directory:
- Create "zdl" directory to store all the SNPE header files in andorid app include path Android_App_Whisper/app/src/main/cpp/inc/
- Copy all SNPE header files from location $SNPE_ROOT/include/SNPE/* to ./Android_App_Whisper/app/src/main/cpp/inc/zdl/
- Take SNPE_ROOT/lib/android/snpe-release.aar file, unzip it
- Then create **app/src/main/jniLibs** folder and paste everything from **jni/arm64-v8a** which is extracted from **snpe-release.aar** to this **app/src/main/jniLibs/arm64-v8a** folder.
- Take SNPE_ROOT/lib/hexagon-v75 if you want to run it on Snapdragon 8th gen 3 device
- Take SNPE_ROOT/lib/hexagon-v73 if you want to run it on Snapdragon 8th gen 2 device. 



* If build process fails with `libSNPE.so` duplication error, then please change its path from "jniLibs" to "cmakeLibs" as follows : `${CMAKE_CURRENT_SOURCE_DIR}/../cmakeLibs/arm64-v8a/libSNPE.so` in `app/src/main/cpp/CMakeList.txt` under `target_link_libraries` section and delete `libSnpe.so` from "jniLibs" directory.


#### Debug Tips
* After installing the application, if it is crashing, try to collect the logs from QIDK device.
* To collect logs run the below commands.
	*	adb logcat -c
	* adb logcat > log.txt
	*	Now, run the app. Once, the app has crashed do Ctrl+C to terminate log collection.
	*	log.txt will be generated in current folder.
	*	Search for the keyword "crash" to analyze the error.

* On opening the app, if Unsigned or Signed DSP runtime is not getting detected, then please search the logcat logs with keywork `dsp` for the FastRPC errors.
* DSP runtime may not get detected due to SE Linux security policy in some Android builds. Please try out following commands to set `permissive` SE Linux policy.
```
adb disable-verity
adb reboot
adb root
adb remount
adb shell setenforce 0
// launch the application
```		
# Result

<p align="center">
<img src="readme-assets/ASR_gif.gif" width=35% height=35%>
</p>


## Qualcomm® Neural Processing SDK C++ APIs JNI Integration

Please refer to SDK Native application tutorial : https://developer.qualcomm.com/sites/default/files/docs/snpe/cplus_plus_tutorial.html

## Credits

The pre-trained model is from HuggingFace Repository by MRM8488 https://huggingface.co/docs/transformers/model_doc/wav2vec2


## References

- https://arxiv.org/abs/2006.11477
- https://huggingface.co/docs/transformers/model_doc/wav2vec2
- https://huggingface.co/facebook/wav2vec2-base-960h
- https://www.tensorflow.org/datasets/catalog/librispeech
- https://developer.qualcomm.com/sites/default/files/docs/snpe/index.html
- https://developer.qualcomm.com/sites/default/files/docs/snpe/setup.html
- https://developer.qualcomm.com/sites/default/files/docs/snpe/cplus_plus_tutorial.html
- https://developer.qualcomm.com/software/qualcomm-neural-processing-sdk/learning-resources/vision-based-ai-use-cases/performance-analysis-using-benchmarking-tools



###### *Qualcomm Neural Processing SDK is a product of Qualcomm Technologies, Inc. and/or its subsidiaries.*
