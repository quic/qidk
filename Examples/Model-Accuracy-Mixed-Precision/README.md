# MIXED_PRECISION
Mixed precision study to attain better accuracy after quantization

# Object Detection with YOLOv8n

## Pre-requisites

* Please follow the instructions for setting up Qualcomm Neural Processing SDK using the [link](https://developer.qualcomm.com/sites/default/files/docs/snpe/setup.html) provided. 
* To install onnx follow the instructions from this [link](https://qdn-drekartst.qualcomm.com/hardware/qualcomm-innovators-development-kit/frameworks-qualcomm-neural-processing-sdk-for-ai)
* Do checkout this [Accuracy_Analyzer_YoloV8.ipynb](Accuracy_Analyzer_YoloV8.ipynb) as quantization of yoloV8n is tricky. The simple conversion won't give any accuracy.
* This demo is prepared using Qualcomm Neural Processing SDK-2.10.0.4541

## How to get the model ? 

For this demo, a PyTorch model YOLOv8n is used. The public repo is mentioned in above table.

## Convert PyTorch model to ONNX
Use below collab notebook to get the ONNX model. In this notebook it will install Ultralytics and its dependencies.<br>
After, that add your code block by using ```code+``` icon and paste this command in it ```!yolo export model=yolov8n.pt imgsz=640 format=onnx opset=11```
```python
https://colab.research.google.com/github/ultralytics/ultralytics/blob/main/examples/tutorial.ipynb#scrollTo=kUMOQ0OeDBJG
```
Once ONNX model is prepapred, put it in the onnx_model folder in current working directory MIXED_PRECISION.

## Convert model to DLC 
Follow the notebook for further steps.
It will show challenges with respect to quantization and will also walk through solution.


# References
1. https://github.com/ultralytics/ultralytics
2. coco 2017 dataset http://images.cocodataset.org/zips/val2017.zip
3. mAP implementation is leveraged from here : https://github.com/Cartucho/mAP


###### *Snapdragon and Qualcomm Neural Processing SDK are products of Qualcomm Technologies, Inc. and/or its subsidiaries.*
