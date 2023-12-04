![Screenshot](./images/logo-quic-on@h68.png)

# Qualcomm® Innovators Development Kit - QIDK

Qualcomm® Innovators Development Kit (QIDK) provides sample applications to demonstrate the capability of Hardware Accelerators for AI, and Software AI stack.

This repository contains sample android applications, which are designed to use components from the following products:

1. [Qualcomm® Neural Processing SDK for AI](https://developer.qualcomm.com/software/qualcomm-neural-processing-sdk)
   Also referred to as SNPE
2. [Qualcomm® AI Engine Direct SDK](https://developer.qualcomm.com/software/qualcomm-ai-engine-direct-sdk)
   Also referred to as QNN
3. [AI Model Efficiency Tool Kit (AIMET)](https://github.com/quic/aimet)
4. [AIMET Model Zoo](https://github.com/quic/aimet-model-zoo)

This Repository is divided into following categories

## QWA Course - AI on Qualcomm Innovators Development Kit 

Qualcomm Wireless Academy has a free course on "AI on Qualcomm Innovators Development Kit".<br><br>
Course Link : https://qwa.qualcomm.com/course-catalog/AI-on-QIDK

This course is geared toward AI application developers, university students, and AI enthusiasts.
This course is applicable, even if a developer is AI SDK on Qualcomm platforms other than QIDK. 

All QIDK deliverables are covered in this course in detail with hands-on lab sessions. 

## QHCI (Qualcomm HTP Customized Interface)

QHCI aims to provide examples for CV algo and QNN UDO implementation based on HVX programming.

QHCI is targetted towards any external developers, starting on Qualcomm platform with QNN and HVX programming. 
QHCI will help with necessary resources, tools to accelerate their workflow. 

QHCI is also targetted to create an open-source HVX development ecosystem.

Link to QHCI public repository : [QHCI](./QHCI/)

## Examples

Contain examples to use features of above SDKs

|   Type    | SDK   |   Details   |   Link |
|  :---:    |    :---:   |    :---:  |   :---:  |
|  Model    | QNN - 2.10.40.4  |  Model - EnhancementGAN | [ReadMe](./Examples/QNN-Model-Example-EnhancementGAN/README.md) |
|  Model    | QNN - 2.10.40.4  |  Model - SESR | [ReadMe](./Examples/QNN-Model-Example-SESR/README.md) |

## Model Enablement

Contains examples for : 

1. Using models, that are not directly supported with AI SDK
2. Debug Quantizatin accuracy loss

|   Type    | SDK   |   Details   |   Link |
|  :---:    |    :---:   |    :---:  |   :---:  |
| Model Conversion Guide | SNPE-2.12 | Model-Accuracy-Mixed-Precision | [ReadMe](./Model-Enablement/Model-Accuracy-Mixed-Precision/README.md)|
| Model Conversion Guide | SNPE-2.12 | Model-Conversion-Layer-Replacement | [ReadMe](./Model-Enablement/Model-Conversion-Layer-Replacement/README.md)|
| Model Conversion Guide | SNPE-2.12 | Model-Conversion-UDO-SELU | [ReadMe](./Model-Enablement/Model-Conversion-UDO-SELU/README.md)|

## Solutions

Contain end-to-end ready-to-run solutions

|   Type     | Solution   |   SDK   |   API   | Model   |   ReadMe |  Demo   |
|  :---:     |    :---:   |    :---:  |    :---:  |    :---:  |   :---:  |  :---:  |
|  NLP       | Question Answering       |  SNPE-2.7 | Native API | Electra-small     |  [ReadMe](./Solutions/NLPSolution1-QuestionAnswering/README.md) |   [Demo](./Solutions/NLPSolution1-QuestionAnswering/README.md#qa-app-workflow)   |
|  NLP       | Sentiment Analysis       |  SNPE-2.7 | Native API | MobileBERT     |  [ReadMe](./Solutions/NLPSolution2-SentimentAnalysis/README.md)  |   [Demo](./Solutions/NLPSolution2-SentimentAnalysis/README.md#sa-app-workflow)   |
|  Vision    | Object Detection       |  SNPE-2.7 |   Java API  | Mobilenet SSD V2    | [ReadMe](./Solutions/VisionSolution1-ObjectDetection/README.md) |   [Demo](./Solutions/VisionSolution1-ObjectDetection/demo/ObjectDetection-Demo.gif)   |
|  Vision    | Object Detection YoloNAS | SNPE-2.12 | Native API | YoloNAS| [ReadMe](./Solutions/VisionSolution1-ObjectDetection-YoloNas/README.md)| [Demo](./Solutions/VisionSolution1-ObjectDetection-YoloNas/demo/ObjectDetectYoloNAS.gif)|
|  Vision    | Image Super Resolution       |SNPE-2.7 |   Java API | SESR XL    | [ReadMe](./Solutions/VisionSolution2-ImageSuperResolution/README.md) |   [Demo](./Solutions/VisionSolution2-ImageSuperResolution/demo/VisionSolution2-ImageSuperResolution.gif)   |
|  Vision    | Image Enhancement       |SNPE-2.7 |  Java API | EnhancedGAN    | [ReadMe](./Solutions/VisionSolution3-ImageEnhancement/README.md)  |   [Demo](./Solutions/VisionSolution3-ImageEnhancement/demo/VisionSolution3-ImageEnhancement.gif)   |
|  Vision    | Pose Estimation |SNPE-2-12| Native API|YoloNAS + HRNet| [ReadMe](./Solutions/VisionSolution4-PoseEstimation/README.md)|[Demo](./Solutions/VisionSolution4-PoseEstimation/demo/PoseDetectionYoloNas.gif)|

## Tools

Contain tools to simplify workflow

|   Tool    | SDK   |   Details   |   Link |
|  :---:    |    :---:   |    :---:  |   :---:  |
|  PySNPE   | SNPE - 2.10  |  Python Interface to 'Qualcomm Neural Processing SDK for AI' tools | [ReadMe](./Tools/pysnpe_utils/README.md) |
|  snpe-docker    | SNPE - 2.10  |  Docker container for 'Qualcomm Neural Processing SDK for AI' | [ReadMe](./Tools/snpe-docker/README.md) |




Pls write to qidk@qti.qualcomm.com for any questions/suggestions

###### *Qualcomm Neural Processing SDK, and Qualcomm Innovators Development Kit are products of Qualcomm Technologies, Inc. and/or its subsidiaries. AIMET is a product of Qualcomm Innovation Center, Inc.*
