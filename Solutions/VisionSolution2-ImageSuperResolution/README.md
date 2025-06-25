# Introduction

### About "Image Super Resolution" 

- Current project is an sample Android application for AI-based Image Super Resolution using <b>Qualcomm® Neural Processing SDK for AI </b> framework. 
- Model used in this sample is : **Collapsible Linear Blocks for Super-Efficient Super Resolution** (*https://arxiv.org/abs/2103.09404*)
- SESR model is also part of AIMET Model Zoo (*https://github.com/quic/aimet-model-zoo/#pytorch-model-zoo*)
- This sample enhances input image resolution by 4x along width, and height. If input resolution is wxh, output resolution will be 4*w x 4*h
- DLC models take only fixed input size. 
- If users intend to modify the input size and/or scale factor of the above model, they need to regenerate model DLC using AI SDK (steps given below)
- If users intend to use a different model in this demo framework, **image pre/post processing will be needed**. 
- Current pre/post processing is specific for the model used. 

### Pre-Requisites 

- Qualcomm® Neural Processing SDK for AI setup should be completed by following the guide here : https://docs.qualcomm.com/bundle/publicresource/topics/80-63442-2/setup.html?product=1601111740010412
- Android Studio to import sample project
- Android NDK to build native code
- Install opencv using ```pip install opencv-python```

# Model Selection, and DLC conversion

### Model Overview

Source : *https://arxiv.org/pdf/2103.09404.pdf*

![Screenshot](./doc/doc-fig1.jpg)

- Proposed SESR at training time contains two 5 × 5 and m 3 × 3 linear blocks. 
- There are two long residuals and several short residuals over 3 × 3 linear blocks. 
- A k × k linear block first uses a k × k convolution to project x input channels to p intermediate channels, which are projected back to y output channels via a 1 × 1 convolution. 
- Short residuals can further be collapsed into convolutions. 
- Final inference time SESR just contains two long residuals and m+2 narrow convolutions, resulting in a VGG-like CNN.

### Steps to convert model to DLC

- Ensure the pre-requisites mentioned at the begining of this page, are completed
- Note: As a general practice, please convert PYTORCH models to ONNX first, and then convert ONNX to DLC.
- Please run .\Genarate_Model\sesr.ipynb notebook to prepare dlc. 


### How to change the model? 

- As mentioned above, current sample is packaged with 128x128 input resolution, and 4x upscale. i.e., output is 512x512
  1. Input Dimensions and Resolution: WidthxHeight = 128x128 Format = RGB
  2. Output Dimensions and Resolution: WidthxHeight = 512x512 Format = RGB 
- If user wants to try with other resolution/upscale factor they need to download respective pre-trained weights from AIMET and generate DLC using above steps. 
- If user wants to try with a different model, then user needs to ensure if the pre-post processing done for SESR is applicable for the new model and modify accordingly. 

# Source Overview

### Source Organization

- <DIR> demo: Contains demo video, GIF 
- <DIR> doc: Contains documentation/images for current project
- <DIR> snpe-release: Contains SDK release binary. It is recommended to ** regenerate DLC and replace the SDK release binary **  if there is a significant change in SDK release over time
- <DIR> superresolution: Contains source files in standard Android app format.

### Code Implementation

***Model Initialization***<br/>
Before You can use the model it has to be initialized first. The initialization process needs these parameters:

- Context: Activity or Application context<br/>
- ModelName: Name of the model that you want to use<br/>
- String run_time: A specific runtime (The alternatives for run_time are CPU, GPU, and DSP).<br/>

```java
    package com.qcom.imagesuperres;
    ...
    
    public class SNPEActivity extends AppCompatActivity {
    public static final String SNPE_MODEL_NAME = "SuperResolution_sesr";
    
    ...
 
        public Result<SuperResolutionResult> process(Bitmap bmps, String run_time){
 
    ....
 
        superResolution = new SuperResolution();

        // Model is initialised in below fun. Model config been set as per AI SDK.

        boolean superresInited = superResolution.initializingModel(this, SNPE_MODEL_NAME, run_time);
        }
    }
```

***Running Model***<br/>
 
After the model is initialized, you can pass a list of bitmaps to be processed.

As mentioned earlier DLC models can work with specific image sizes.
Therefore, we need to resize the input image to the size accepted by DLC before passing image to DLC

This source code uses an operator to aid users in image pre-processing. 
sizeOperation parameter to the Process method, can be used to pre-process the image as shown below:<br/>

Note: If user is already passing exact input size 128x128 sizeOperation should be set to 1. 
 
- 0: Without changes<br/>
- 1: `if (IMAGE_WIDTH = INPUT_HEIGHT and IMAGE_HEIGHT = INPUT_WIDTH)` (user passing input size as required by model) <br/>
- 2: `if ((IMAGE_WIDTH / INPUT_WIDTH) = (IMAGE_HEIGHT / INPUT_HEIGHT))`(input needs to be scaled)  <br/>

```java
    int sizeOperation = 1;

        // Function to process the enhancement operation

        result = enhancement.process(new Bitmap[] {bmps}, sizeOperation);
```
<br/>***Results***
<br/>
- The processed results are returned in a `Result` object of generic type `SuperResolutionResult`. 
- The `SuperResolutionResult` contains an array of bitmaps each representing an Enhancement process (In the sample there is only one bitmap). 
- Along with this list, there is also the `inferenceTime` of the process for performance information.



<br/>***Release***
<br/>
Since the model initiation process happens on the native side it will not be garbage collected; therefore,
you need to release the model after you are done with it.<br/>

```java
    superResolution.freeNetwork();
```

# Build and run with Android Studio

## Build APK file with Android Studio 

1. Clone QIDK repo. 
2. Generate DLC using the steps mentioned above (super_resolution_sesr_opt.dlc)
3. Copy "snpe-release.aar" file from android folder in "Qualcomm Neural Processing SDK for AI" release from Qualcomm Developer Network into this folder : VisionSolution2-ImageSuperResolution\snpe-release\
   Command: cp $SNPE_ROOT/lib/android/snpe-release.aar snpe-release
4. Copy DLC generated from the notebook at : VisionSolution2-ImageSuperResolution\superresolution\src\main\assets\ (super_resolution_sesr_opt.dlc)
   Command: cp -R Genarate_Model/models/*.dlc ./superresolution/src/main/assets/
5. Import folder VisionSolution2-ImageSuperResolution as a project in Android Studio 
6. Compile the project. 
7. Output APK file should get generated : superresolution-debug.apk
8. Prepare the Qualcomm Innovators development kit to install the application (Do not run APK on emulator)

If Unsigned or Signed DSP runtime is not getting detected, then please check the logcat logs for the FastRPC error. DSP runtime may not get detected due to SE Linux security policy. Please try out following commands to set permissive SE Linux policy.
```java
adb disable-verity
adb reboot
adb root
adb remount
adb shell setenforce 0
```

9. Install and test application : superresolution-debug.apk

```java
adb install -r -t superresolution-debug.apk
```

10. launch the application

Following is the basic "Image Super Resolution" Android App 

1. Select one of the given images from the drop-down list
2. Select the run-time to run the model (CPU, GPU or DSP)
3. Observe the result of model on screen
4. Also note the performance indicator for the particular run-time in mSec

Same results for the application are shown below 

# Results

- Demo video, and performance details as seen below:

![Screenshot](./demo/VisionSolution2-ImageSuperResolution.gif)

# References

    1. Collapsible Linear Blocks for Super-Efficient Super Resolution - https://arxiv.org/abs/2103.09404
    2. SESR at AIMET model zoo - https://github.com/quic/aimet-model-zoo/#pytorch-model-zoo


###### *Qualcomm Neural Processing SDK and Snapdragon are products of Qualcomm Technologies, Inc. and/or its subsidiaries. AIMET Model Zoo is a product of Qualcomm Innovation Center, Inc.*
