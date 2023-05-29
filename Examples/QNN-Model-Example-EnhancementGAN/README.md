# "Image Enhancement" using EnhancedGAN

## Pre-Requisites 

- Qualcomm® AI Engine Direct SDK setup should be completed
- Install DNN dependencies for SDK
- Install opencv using ```pip install opencv-python```

## How to get the model ? 

Download ONNX model required for this tutorial

```python
wget https://github.com/arsenyinfo/EnlightenGAN-inference/raw/main/enlighten_inference/enlighten.onnx -P onnx-model/
```

Optional command to simplify the onnx model.
```python
onnxsim onnx-model/enlighten.onnx onnx-model/enlighten_opt.onnx
```

## Steps to convert model to FP_32 model
- Copy the **onnx model / simplified onnx-model** you get from above steps to ```onnx-model``` folder (by default it has been already provided) and run the following command :
```python
qnn-onnx-converter --input_network onnx-model/enlighten_opt.onnx --input_dim input 1,3,240,320 --out_node output --output_path FP_32/enlighten_opt.cpp
```
This will produce the following artifacts:
  ```
    FP_32/enlighten_opt.cpp 
    FP_32/enlighten_opt_net.json
    FP_32/enlighten_opt.bin
  ```
The artifacts include .cpp file containing the sequence of API calls, and a .bin file containing the static data associated with the model.
   
Once the model is converted it is built with ``` qnn-model-lib-generator ``` :
```python
qnn-model-lib-generator -c FP_32/enlighten_opt.cpp -b FP_32/enlighten_opt.bin -o model_libs/FP_32
```   
This will produce the following artifacts:

```
   model_libs/FP_32/aarch64-android/libenlighten_opt.so
   model_libs/FP_32/arm-android/libenlighten_opt.so
   model_libs/FP_32/x86_64-linux-clang/libenlighten_opt.so
```

## Quantization of the Model
- Quantization can improve model performance in terms of latency and make the model light weight. 
change directory to Model/SESR/Quantization and run below command:
```python
qnn-onnx-converter --input_network ../onnx-model/enlighten_opt.onnx --input_dim input 1,3,240,320 --out_node output --output_path INT_8/enlighten_opt_quantized.cpp --input_list list.txt
```
This will produce the following artifacts:
  ```
    Quantization/INT_8/enlighten_opt_quantized.cpp
    Quantization/INT_8/enlighten_opt_quantized_net.json
    Quantization/INT_8/enlighten_opt_quantized.bin
  ```
The artifacts include .cpp file containing the sequence of API calls, and a .bin file containing the static data associated with the model.
   
Once the model is converted it is built with ``` qnn-model-lib-generator ``` :
```python
mkdir model_libs
qnn-model-lib-generator -c INT_8/enlighten_opt_quantized.cpp -b INT_8/enlighten_opt_quantized.bin -o ../model_libs/INT_8/
```   
This will produce the following artifacts:

```
   ../model_libs/INT_8/aarch64-android/libenlighten_opt_quantized.so
   ../model_libs/INT_8/arm-android/libenlighten_opt_quantized.so
   ../model_libs/INT_8/x86_64-linux-clang/libenlighten_opt_quantized.so
```
**Note :** _By default libraries are built for all targets. To compile for a specific target, use the -t <target> option with qnn-model-lib-generator. Choices of <target> are aarch64-android, arm-android, and x86_64-linux-clang._

One distinction is the HTP backend requires a Quantized model. Additionally, running the HTP on device requires the generation of a serialized context. To generate the context run:
```
qnn-context-binary-generator --backend ${QNN_SDK_ROOT}/target/x86_64-linux-clang/lib/libQnnHtp.so --model model_libs/INT_8/x86_64-linux-clang/libenlighten_opt_quantized.so --binary_file lib_graph_prepare_from_int8_x86.serialized
```
   
# Model_Prototyping
- To check the execution on snapdragon device, please run "[Model_Prototyping_EnhancementGAN.ipynb](Prototyping/Model_Prototyping_EnhancementGAN.ipynb)" a jupyter notebook present in the Prototyping folder.
- To run any jupyter notebook, run the below command. It will generate few links on the screen, pick the link with your machine name on it (host-name) and paste it in any browser.
- Navigate to the notebook ".ipynb" file and simply click that file.
```python
jupyter notebook --no-browser --port=8080 --ip 0.0.0.0 --allow-root
```

# References
  1. Jiang, Y., Gong, X., Liu, D., Cheng, Y., Fang, C., Shen, X., Yang, J., Zhou, P. and Wang, Z., 2021. Enlightengan: Deep light enhancement without paired supervision. IEEE Transactions on Image Processing, 30, pp.2340-2349.
  2. https://github.com/arsenyinfo/EnlightenGAN-inference

###### *For any queries/feedback, please write to qidk@qti.qualcomm.com*
###### *Qualcomm Neural Processing SDK, Qualcomm AI Engine Direct, and Snapdragon are products of Qualcomm Technologies, Inc. and/or its subsidiaries.*
###### *AIMET Model Zoo is a product of Qualcomm Innovation Center, Inc.*
