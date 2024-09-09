# HRNET pose-estimation
The project is designed to utilize the <b>Qualcomm® Neural Processing SDK for AI </b>, a deep learning software from Snapdragon platforms for Pose Detection in Android. The Android application can be designed to use any built-in/connected camera to capture the objects and use Machine Learning model to get the pose of any human present in the camera feed. This solution uses two opensource models to achieve better accuracy on Human Pose Estimation. We use YoloNAS_SSD to identify each person in a frame and then give this data to HRNET model to get pose estimation on the previously identified person.


# Pre-requisites

* Before starting the Android application, please follow the instructions for setting up Qualcomm Neural Processing SDK using the link provided. https://docs.qualcomm.com/bundle/publicresource/topics/80-63442-2/setup.html?product=1601111740010412
* Android device 6.0 and above which uses below mentioned Snapdragon processors/Snapdragon HDK with display can be used to test the application 
* Download CocoDataset 2017 and give its path to Generate_DLC.ipynb. Change variable "dataset_path" during Quantization for both models.

## List of Supported Devices

- Snapdragon® SM8550
- Snapdragon® SM8650

The above targets supports the application with CPU, GPU and DSP. For more information on the supported devices, please follow this link https://docs.qualcomm.com/bundle/publicresource/topics/80-63442-2/overview.html?product=1601111740010412
  
# Source Overview

## Source Organization

demo : Contains demo GIF

doc : Contains documentation/images for current project

app : Contains source files in standard Android app format

app\src\main\assets : Contains Model binary DLC

app\src\main\java\com\qc\posedetectionYoloNAS : Application java source code 

app\src\main\cpp : native source code 
  
sdk: Contains openCV sdk

## DLC Generation

Run jupyter notebook Generate_models/GenerateDLC.ipynb. This notebook will generate 2 dlc(s).
	
*  Quantized YoloNAS SSD model as Quant_yoloNas_s_320.dlc
*  Quantized HRNET  model as hrnet_axis_int8.dlc
	
## Code Implementation

This application opens a camera preview, collects all the frames and converts them to bitmap. 
Here two network are built via Neural Network builder by passing Quant_yoloNas_s_320.dlc and hrnet_axis_int8.dlc as the input. 
The bitmap goes through some preprocessing and then given to the YoloNAS model for identifying human and give coordintes for box containing that human.
When human is identified, we process the frame to blackout all the data except the box containing human to HRNET model, which then returns coords for a human pose. 

### Prerequisite for Camera Preview.

Permission to obtain camera preview frames is granted in the following file:
```python
/app/src/main/AndroidManifest.xml
<uses-permission android:name="android.permission.CAMERA" />
 ```
In order to use camera2 APIs, add the below feature
```python
<uses-feature android:name="android.hardware.camera2" />
```
### Loading Model
Code snippet for neural network connection and loading model:
```java
snpe = snpeBuilder.setOutputLayers({})
            .setPerformanceProfile(zdl::DlSystem::PerformanceProfile_t::BURST)
            .setExecutionPriorityHint(
                    zdl::DlSystem::ExecutionPriorityHint_t::HIGH)
            .setRuntimeProcessorOrder(runtimeList)
            .setUseUserSuppliedBuffers(useUserSuppliedBuffers)
            .setPlatformConfig(platformConfig)
            .setInitCacheMode(useCaching)
            .setCPUFallbackMode(true)
            .setUnconsumedTensorsAsOutputs(true)
            .build();  //Buids the Network
```
### Object-Detect Pre-Processing
The bitmap image is passed as openCV Mat to native and then converted to BGR Mat of size 320x320x3. Basic image processing depends on the kind of input shape required by the model, then writing that processed image into application buffer. Object Detection post processing
```java
    cv::Mat img320;
    cv::resize(img,img320,cv::Size(320,320),cv::INTER_LINEAR);  

    float inputScale = 0.00392156862745f;    //normalization value, this is 1/255

    float * accumulator = reinterpret_cast<float *> (&dest_buffer[0]);

    //opencv read in BGRA by default
    cvtColor(img320, img320, CV_BGRA2BGR);
    LOGI("num of channels: %d",img320.channels());
    int lim = img320.rows*img320.cols*3;
    for(int idx = 0; idx<lim; idx++)
        accumulator[idx]= img320.data[idx]*inputScale;
 ```
 
### Object-Detect Post-Processing

This included getting the class with highest confidence for each 2100 boxes and applying Non-Max Suppression to remove overlapping boxes.
```python
for(int i =0;i<(2100);i++)  
   {
       int start = i*80;
       int end = (i+1)*80;

       auto it = max_element (BBout_class.begin()+start, BBout_class.begin()+end);
       int index = distance(BBout_class.begin()+start, it);

       std::string classname = classnamemapping[index];
       if(*it>=0.5 )
       {
           int x1 = BBout_boxcoords[i * 4 + 0];
           int y1 = BBout_boxcoords[i * 4 + 1];
           int x2 = BBout_boxcoords[i * 4 + 2];
           int y2 = BBout_boxcoords[i * 4 + 3];
           Boxlist.push_back(BoxCornerEncoding(x1, y1, x2, y2,*it,classname));
       }
   }

   std::vector<BoxCornerEncoding> reslist = NonMaxSuppression(Boxlist,0.20);
```
then we just scale the coords for original image
```python
float top,bottom,left,right;
        left = reslist[k].y1 * ratio_1;   //y1
        right = reslist[k].y2 * ratio_1;  //y2

        bottom = reslist[k].x1 * ratio_2;  //x1
        top = reslist[k].x2 * ratio_2;   //x2
```
                              
### Pose Detection
The bitmap image is passed as openCV Mat to native and then converted to BGR Mat of size 256x192x3. Basic image processing depends on the kind of input shape required by the model, then writing that processed image into application buffer. Based on box coordinates we receive, we transform the image, so only pixels which are contained in the box are lit, all other pixels are blackened. This is done to hide any other human present in the frame, as HRNET is designed to work best with single person.
                              
```java
    //Preprocessing
    getcenterscale(img.cols, img.rows, center, scale, bottom, left, top, right); //get center and scale based on Box coordinates
    cv::Mat trans = get_affine_transform(HRNET_model_input_height, HRNET_model_input_width, 0, center, scale);
    cv::Mat model_input(HRNET_model_input_width,HRNET_model_input_height, img.type());
    cv::warpAffine(img, model_input, trans,model_input.size(), cv::INTER_LINEAR);
    cvtColor(model_input, model_input, CV_BGRA2BGR);  //Changing num of channels

    //Writing into buffer for inference
    int lim = model_input.rows*model_input.cols*3;
    float * accumulator = reinterpret_cast<float *> (&dest_buffer[0]);
    for(int idx = 0; idx<lim; ){
        accumulator[idx]= (float) (((model_input.data[idx] / 255.0f) - 0.485f) / 0.229f);
        accumulator[idx+1] = (float) (((model_input.data[idx+1] / 255.0f) - 0.456f) / 0.224f);
        accumulator[idx+2] = (float) (((model_input.data[idx+2] / 255.0f) - 0.406f) / 0.225f);
        idx=idx+3;
    }

 ```

### Drawing Pose on camera feed 
Model returns the respective Float Tensors, from which the shape of the object and its name can be inferred. Canvas is used to draw a rectangle for all identified humans and their pose.
```java
   for (int i = 0; i < Connections.length; i++) {
                int kpt_a = Connections[i][0];
                int kpt_b = Connections[i][1];
                int x_a = (int) coords[kpt_a][0];
                int y_a = (int) coords[kpt_a][1];
                int x_b = (int) coords[kpt_b][0];
                int y_b = (int) coords[kpt_b][1];
                if ((x_a | y_a) != 0) {
                    canvas.drawCircle(x_a, y_a, 8, mPosepaint);
                    if ((x_b | y_b) != 0) {
                        canvas.drawCircle(x_b, y_b, 8, mPosepaint);
                        canvas.drawLine(x_a, y_a, x_b, y_b, mPosepaint);
                    }
                }
```

# Build and run with Android Studio

## Build APK file with Android Studio 

1. Clone QIDK repo. 

2. Run below script, from the directory where it is present, to resolve dependencies of this project.

* This will copy snpe-release.aar file from $SNPE_ROOT to "snpe-release" directory in Android project.

	**NOTE - If you are using SNPE version 2.11 or greater, please change following line in resolveDependencies.sh.**
	```
	From: cp $SNPE_ROOT/android/snpe-release.aar snpe-release
	To : cp $SNPE_ROOT/lib/android/snpe-release.aar snpe-release
	```
* Download opencv and paste to sdk directory, to enable OpenCv for android Java.

```java
    sed -i 's/\r$//' resolveDependencies.sh
	bash resolveDependencies.sh
```

	
3. Run jupyter notebook Generate_models/GenerateDLC.ipynb to generate DLC(s) for YoloNAS_SSD, HRNET for hrnet_axis_int8.dlc. Change dataset_path with Coco Dataset Path.
				  
* This script generates required dlc(s) and paste them to appropriate location. 


4. Do gradle sync
5. Compile the project. 
6. Output APK file should get generated : app-debug.apk
7. Prepare the Qualcomm Innovators development kit to install the application (Do not run APK on emulator)

8. If Unsigned or Signed DSP runtime is not getting detected, then please check the logcat logs for the FastRPC error. DSP runtime may not get detected due to SE Linux security policy. Please try out following commands to set permissive SE Linux policy.

It is recommended to run below commands.

```java
adb disable-verity
adb reboot
adb root
adb remount
adb shell setenforce 0
```

9. Install and test application : app-debug.apk
```java
adb install -r -t app-debug.apk
```

10. launch the application

Following is the basic "Pose Detection Yolo NAS" Android App 

1. On first launch of application, user needs to provide camera permissions
2. Camera will open and pose will be seen, if human is visible on the screen 
3. User can select appropriate run-time for model, and observe performance difference

Same results for the application are : 

## Demo of the application
![Screenshot](.//demo/PoseDetectionYoloNas.gif)

# References
1. SSD - Single shot Multi box detector - https://arxiv.org/pdf/1512.02325.pdf
2. https://github.com/Deci-AI/super-gradients
3. https://zenodo.org/record/7789328
4. HRNET model - Deep High-Resolution Representation Learning for Visual Recognition - https://arxiv.org/abs/1908.07919
5. https://github.com/leoxiaobin/deep-high-resolution-net.pytorch

	
###### *Snapdragon and Qualcomm Neural Processing SDK are products of Qualcomm Technologies, Inc. and/or its subsidiaries.*