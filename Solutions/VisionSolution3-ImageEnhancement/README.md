# Introduction

### About "Image Enhancement"

- Current project is an sample Android application for AI-based Image Enhancement using <b>Qualcomm® Neural Processing SDK for AI </b> framework.
- Model used in this sample is : EnlightenGAN <https://arxiv.org/abs/1906.06972>
- This solution can take variable size input image and after preprocessing it can apply the enhancement model to the image.
- If users intend to use a different model in this demo framework, they need to also take care of image pre/post processing.
- Current pre/post processing is specific for the model used.

### Pre-Requisites

- Qualcomm® Neural Processing SDK setup should be completed by following the guide here:<br> https://docs.qualcomm.com/bundle/publicresource/topics/80-63442-2/setup.html?product=1601111740010412
- Android Studio to import sample project
- Android NDK to build native code

# Model Selection, and DLC conversion

### Model Overview

Source : *https://arxiv.org/abs/1906.06972*
![image](https://github.qualcomm.com/storage/user/14627/files/e5687aea-6d0d-4f4d-8875-df022809fea6)

EnlightenGAN proposes a highly effective unsupervised generative adversarial network, dubbed EnlightenGAN, that can be trained without low/normal-light image pairs, yet proves to generalize very well on various real-world test images. Instead of supervising the learning using ground truth data, it proposes to regularize the unpaired training using the information extracted from the input itself, and benchmark a series of innovations for the low-light image enhancement problem, including a global-local discriminator structure, a self-regularized perceptual loss fusion, and the attention mechanism. Through extensive experiments, our proposed approach outperforms recent methods under a variety of metrics in terms of visual quality and subjective user study. Thanks to the great flexibility brought by unpaired training, EnlightenGAN is demonstrated to be easily adaptable to enhancing real-world images from various domains.


### Steps to convert model to DLC
- Ensure the pre-requisites mentioned at the beginning of this page, are completed
- Setup ONNX on your machine.
- Download pre-trained onnx model from this repo  - https://github.com/arsenyinfo/EnlightenGAN-inference/tree/main/enlighten_inference

```python
wget https://github.com/arsenyinfo/EnlightenGAN-inference/raw/main/enlighten_inference/enlighten.onnx
onnxsim enlighten.onnx enlighten_opt.onnx
```

- Convert the onnx model to DLC with below command. Below, command will also fix the input dimension for the dlc. 

```python
snpe-onnx-to-dlc -i enlighten_opt.onnx -d input 1,3,240,320 -o enlighten_fixed.dlc
```

- For better performance on Snapdragon SM8750 model quantization is recommended. For quantization please keep the dlc, data and list.txt in one directory. 
"data" has some raw input for model quantization.
"list.txt" has names of all files in data directory

```python
snpe-dlc-quantize --input_dlc enlighten_fixed.dlc --input_list list.txt --use_enhanced_quantizer --use_adjusted_weights_quantizer --axis_quant --output_dlc enlight_axisQ_cached.dlc --enable_htp --htp_socs sm8750
```

# Source Overview

### Source Organization

- <DIR> demo: Contains demo video, GIF 
- <DIR> doc: Contains documentation/images for current project
- <DIR> snpe-release: Contains AI SDK release binary form AI SDK release. It is recommended to ** regenerate DLC and replace the AI SDK release binary **  if there is a significant change in AI SDK release over time.
- <DIR> enhancement: Contains source files in standard Android app format.

### Code Implementation

***Model Initialization***<br/>
Before You can use the model it has to be initialized first. The initialization process needs these parameters:

- Context: Activity or Application context<br/>
- ModelName: Name of the model that you want to use<br/>
- String runtime: A specific runtime (The alternatives for run_time are CPU, GPU_FLOAT16, and DSP).<br/>

```java
    package com.qcom.enhancement;
    ...
    
    public class SNPEActivity extends AppCompatActivity {
    public static final String ENHANCEMENT_SNPE_MODEL_NAME = "enlight_axisQ_cached";
    
    ...
 
        public Result<EnhancedResults> process(Bitmap bmps, String run_time){
 
    ....
 
        enhancOps = new EnhancOps();

        // Model is initialised in below fun. Model config been set as per AI SDK.
        boolean enhancementInited = enhancOps.initializingModel(this, ENHANCEMENT_SNPE_MODEL_NAME, run_time);
        }
    }
```

***Running Model***<br/>

After the model is initialized, you can pass a list of bitmaps to be processed.

As mentioned earlier DLC models can work with specific image sizes.
Therefore, we need to resize the input image to the size accepted by DLC before passing image to DLC

This source code uses an operator to aid users in image pre-processing.
sizeOperation parameter to the Process method, can be used to pre-process the image as shown below:<br/>

Note: If user is already passing exact input size 240x320 sizeOperation should be set to 1.

- 0: Without changes<br/>
- 1: `if (IMAGE_WIDTH = INPUT_HEIGHT and IMAGE_HEIGHT = INPUT_WIDTH)` (user passing input size as required by model) <br/>
- 2: `if ((IMAGE_WIDTH / INPUT_WIDTH) = (IMAGE_HEIGHT / INPUT_HEIGHT))`(input needs to be scaled)  <br/>

```java
    int sizeOperation = 1;

        // Function to process the enhancement operation
        result = enhancOps.process(new Bitmap[] {bmps}, sizeOperation);
```
<br/>***Results***
<br/>
- The processed results are returned in a `Result` object of generic type `EnhancedResults`.
- The `EnhancedResults` contains an array of bitmaps each representing an Enhancement process (In the sample there is only one bitmap).
- Along with this list, there is also the `inferenceTime` of the process for performance information.


<br/>***Release***
<br/>
Since the model initiation process happens on the native side it will not be garbage collected; therefore,
you need to release the model after you are done with it.<br/>

```java
    enhancOps.freeNetwork();
```
# Build and run with Android Studio

## Build APK file with Android Studio 

1. Clone QIDK repo. 
2. Generate DLC using the steps mentioned above (enlight_axisQ_cached.dlc)
3. Copy "snpe-release.aar" file from android folder in "Qualcomm Neural Processing SDK for AI" release from Qualcomm Developer Network into this folder : VisionSolution3-ImageEnhancement\snpe-release\
4. Copy DLC generated in step-2 at : VisionSolution3-ImageEnhancement\enhancement\src\main\assets\ (enlight_axisQ_cached.dlc)
5. Import folder VisionSolution3-ImageEnhancement as a project in Android Studio 
6. Compile the project. 
7. Output APK file should get generated : enhancement-debug.apk
8. Prepare the Qualcomm Innovators development kit to install the application (Do not run APK on emulator)

If Unsigned or Signed DSP runtime is not getting detected, then please check the logcat logs for the FastRPC error. DSP runtime may not get detected due to SE Linux security policy. Please try out following commands to set permissive SE Linux policy.
```java
adb disable-verity
adb reboot
adb root
adb remount
adb shell setenforce 0
```
9. Install and test application : enhancement-debug.apk
```java
adb install -r -t enhancement-debug.apk
```

10. launch the application

Following is the basic "Image Enhancement" Android App 

1. Select one of the given images from the drop-down list
2. Select the run-time to run the model (CPU, GPU or DSP)
3. Observe the result of model on screen
4. Also note the performance indicator for the particular run-time in mSec

Same results for the application are : 

# Results

- Demo video, and performance details as seen below:

![Screenshot](./demo/VisionSolution3-ImageEnhancement.gif)

# References

1. Jiang, Y., Gong, X., Liu, D., Cheng, Y., Fang, C., Shen, X., Yang, J., Zhou, P. and Wang, Z., 2021. Enlightengan: Deep light enhancement without paired supervision. IEEE Transactions on Image Processing, 30, pp.2340-2349.
2. https://github.com/arsenyinfo/EnlightenGAN-inference

    
###### *Qualcomm Neural Processing SDK and Snapdragon are products of Qualcomm Technologies, Inc. and/or its subsidiaries.*    
