# Image Super resolution with SESR

## Pre-Requisites 

- Qualcomm® AI Engine Direct SDK setup should be completed
- Install DNN dependencies for SDK
- Install opencv using ```pip install opencv-python```

## How to get the model ? 

- Ensure the pre-requisites mentioned at the begining of this page, are completed
- Note: As a general practice, please convert PYTORCH models to ONNX first, and then convert ONNX to DLC.
- To convert PYTORCH to ONNX clone the [AIMET-MODEL-ZOO](https://github.com/quic/aimet-model-zoo.git) repo. For, below steps we are assuming that the repo is pointing to this [header](https://github.com/quic/aimet-model-zoo/tree/b7a21a02f3a33387548f376bdf7831b4cc5cc41b).
```python 
git clone https://github.com/quic/aimet-model-zoo.git
cd aimet-model-zoo/
git reset --hard b7a21a02f3a33387548f376bdf7831b4cc5cc41b
```
- Make below changes in this python file : [inference.py](https://github.com/quic/aimet-model-zoo/blob/b7a21a02f3a33387548f376bdf7831b4cc5cc41b/zoo_torch/examples/superres/utils/inference.py). Path: aimet-model-zoo\zoo_torch\examples\superres\utils\inference.py. 
   - comment below imports : <br>
      #from aimet_torch.quantsim import QuantizationSimModel <br>
      #from aimet_torch.qc_quantize_op import QuantScheme
   - Replace this block of code: 

```python
with torch.no_grad():
       sr_img = model(img_lr.unsqueeze(0).to(device)).squeeze(0)
images_sr.append(post_process(sr_img))
```
   With below block   

```python
# With this block
 with torch.no_grad():
        sr_img = model(img_lr.unsqueeze(0).to(device)).squeeze(0)
        input_shape = [1, 3, 128, 128]
        input_data = torch.randn(input_shape)
        torch.onnx.export(model, input_data, "super_resolution.onnx", export_params=True,
                          opset_version=11, do_constant_folding=True, input_names = ['lr'], output_names = ['sr'])
 images_sr.append(post_process(sr_img))
```

- Make below changes in this jupyter notebook : [superres_quanteval.ipynb](https://github.com/quic/aimet-model-zoo/blob/b7a21a02f3a33387548f376bdf7831b4cc5cc41b/zoo_torch/examples/superres/notebooks/superres_quanteval.ipynb). Path: aimet-model-zoo\zoo_torch\examples\superres\notebooks\superres_quanteval.ipynb
   - comment below imports : <br>
      #from aimet_torch.quantsim import QuantizationSimModel <br>
      #from aimet_torch.qc_quantize_op import QuantScheme
   - Set below variables as mentioned
      - DATA_DIR = './data' <br>
      - DATASET_NAME = 'Set14' <br>
      - use_cuda = False <br>
      - model_index = 6
   - Now create directory- aimet-model-zoo\zoo_torch\examples\superres\notebooks\data\Set14 and put any one image in this folder (.jpg). This is required to run the notebook.
   - In section, <I>"Create model instance and load weights"</I> comment all initialization except <I>"model_original_fp32"</I>
   - In section, <I>"Model Inference"</I> comment all initialization except <I>"IMAGES_SR_original_fp32"</I>
- Please, ignore the last block of code in superres_quanteval.ipynb 
- Now, run superres_quanteval.ipynb. Changes made in inference.py will be used in superres_quanteval.ipynb
   
- While running the scripts the code will automatically download pre-trained weights "release_sesr_xl_2x.tar.gz" from AIMET model zoo - https://github.com/quic/aimet-model-zoo/releases/tag/sesr-checkpoint-pytorch

- With above step, ONNX model should get generated. 
- Convert the ONNX model to FP_32 model with below command. Please note that we locked in input dimensions in above code

## Steps to convert model to FP_32 model
- Copy the onnx model you get from above steps to ```onnx_model``` folder (by default it has been already provided)
```python
qnn-onnx-converter --input_network onnx_model/super_resolution.onnx --input_dim lr  1,3,128,128 --out_node sr --output_path FP_32/super_resolution.cpp
```
This will produce the following artifacts:
  ```
    FP_32/super_resolution.cpp
    FP_32/super_resolution.json
    FP_32/super_resolution.bin
  ```
The artifacts include .cpp file containing the sequence of API calls, and a .bin file containing the static data associated with the model.
   
Once the model is converted it is built with ``` qnn-model-lib-generator ``` :
```python
qnn-model-lib-generator -c FP_32/super_resolution.cpp -b FP_32/super_resolution.bin -o model_libs/FP_32
```   
This will produce the following artifacts:

```
   model_libs/FP_32/aarch64-android/libsuper_resolution.so
   model_libs/FP_32/arm-android/libsuper_resolution.so
   model_libs/FP_32/x86_64-linux-clang/libsuper_resolution.so
```

## Quantization of the Model
- Quantization can improve model performance in terms of latency and make the model light weight. 
change directory to Model/SESR/Quantization and run below command:
```python
qnn-onnx-converter --input_network ../onnx_model/super_resolution.onnx --input_dim lr  1,3,128,128 --out_node sr --output_path INT_8/super_resolution_quantized.cpp --input_list list.txt
```
This will produce the following artifacts:
  ```
    Quantization/INT_8/super_resolution_quantized.cpp
    Quantization/INT_8/super_resolution_quantized.json
    Quantization/INT_8/super_resolution_quantized.bin
  ```
The artifacts include .cpp file containing the sequence of API calls, and a .bin file containing the static data associated with the model.
   
Once the model is converted it is built with ``` qnn-model-lib-generator ``` :
```python
qnn-model-lib-generator -c INT_8/super_resolution_quantized.cpp -b INT_8/super_resolution_quantized.bin -o ../model_libs/INT_8/
```   
This will produce the following artifacts:

```
   ../model_libs/INT_8/aarch64-android/libsuper_resolution_quantized.so
   ../model_libs/INT_8/arm-android/libsuper_resolution_quantized.so
   ../model_libs/INT_8/x86_64-linux-clang/libsuper_resolution_quantized.so
```
**Note**
```
By default libraries are built for all targets. To compile for a specific target, use the -t <target> option with qnn-model-lib-generator. Choices of <target> are aarch64-android, arm-android, and x86_64-linux-clang.
```
One distinction is the HTP backend requires a Quantized model. Additionally, running the HTP on device requires the generation of a serialized context. To generate the context run:
```
qnn-context-binary-generator --backend ${QNN_SDK_ROOT}/target/x86_64-linux-clang/lib/libQnnHtp.so --model model_libs/INT_8/x86_64-linux-clang/libsuper_resolution_quantized.so --binary_file lib_graph_prepare_from_int8_x86.serialized
```
   
# Model_Prototyping
- To check the execution on snapdragon device, please run "[Model_Prototyping_SESR.ipynb](Prototyping/Model_Protoyping_SESR.ipynb)" a jupyter notebook present in the Prototyping folder.
- To run any jupyter notebook, run the below command. It will generate few links on the screen, pick the link with your machine name on it (host-name) and paste it in any browser.
- Navigate to the notebook ".ipynb" file and simply click that file.
```python
jupyter notebook --no-browser --port=8080 --ip 0.0.0.0 --allow-root
```

# References

    1. Collapsible Linear Blocks for Super-Efficient Super Resolution - https://arxiv.org/abs/2103.09404
    2. SESR at AIMET model zoo - https://github.com/quic/aimet-model-zoo/#pytorch-model-zoo


###### *For any queries/feedback, please write to qidk@qti.qualcomm.com*
###### *Qualcomm Neural Processing SDK, Qualcomm AI Engine Direct, and Snapdragon are products of Qualcomm Technologies, Inc. and/or its subsidiaries.*
###### *AIMET Model Zoo is a product of Qualcomm Innovation Center, Inc.*