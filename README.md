![Screenshot](./images/logo-quic-on@h68.png)

# Qualcomm® Innovators Development Kit - QIDK

Qualcomm® Innovators Development Kit (QIDK) provides sample applications to demonstrate the capability of Hardware Accelerators for AI, and Software AI stack.

This repository contains sample android applications, which are designed to use components from the following products:

1. [Qualcomm® Neural Processing SDK for AI](https://docs.qualcomm.com/bundle/publicresource/topics/80-63442-2/introduction.html?product=1601111740010412)
   Also referred to as SNPE
2. [Qualcomm® AI Engine Direct SDK](https://docs.qualcomm.com/bundle/publicresource/topics/80-63442-50/introduction.html?product=1601111740009302)
   Also referred to as QNN
3. [AI Model Efficiency Tool Kit (AIMET)](https://github.com/quic/aimet)
4. [AIMET Model Zoo](https://github.com/quic/aimet-model-zoo)

Contents of this repository are verified on Snapdragon 8 Gen2, and Snapdragon 8 Gen3 platforms. 
If users want to try this content on other Qualcomm platforms - please do check with the support e-mail mentioned below. 

This Repository is divided into following categories

## QWA Course - AI on Qualcomm Innovators Development Kit 

Qualcomm Wireless Academy has a free course on "AI on Qualcomm Innovators Development Kit".<br><br>
Course Link : https://qwa.qualcomm.com/course-catalog/AI-on-QIDK

This course is geared toward AI application developers, university students, and AI enthusiasts.
This course is applicable, even if a developer is AI SDK on Qualcomm platforms other than QIDK. 

All QIDK deliverables are covered in this course in detail with hands-on lab sessions. 

### Download AI SDK 

Please note the change in steps to download AI SDK (Steps in QWA course will be modified later)
Users need to follow below procedure do download AI SDK. 
1. Download sdk from this link https://softwarecenter.qualcomm.com/api/download/software/qualcomm_neural_processing_sdk/v2.25.0.240728.zip

Or

1. Visit qpm.qualcomm.com
2. Download AI SDK for Linux (Linux is the host platform for development, QIDK has Android as target platform)

   ![image](https://github.com/user-attachments/assets/dbea8590-1af4-496b-8332-81d1f5640401)


4. Follow SDK documentation for setup, or refer to QWA course.
5. Once SDK is setup, please revert back to QWA course for model conversion, and deployment. 

## Examples

Contain examples to use features of above SDKs

|   Type    | SDK   |   Details   |   Link |
|  :---:    |    :---:   |    :---:  |   :---:  |
|  Model    | AI Engine Direct |  Model - EnhancementGAN | [ReadMe](./Examples/QNN-Model-Example-EnhancementGAN/README.md) |
|  Model    | AI Engine Direct  |  Model - SESR | [ReadMe](./Examples/QNN-Model-Example-SESR/README.md) |
| Android App | NA | Python pre/post in Android App | [ReadMe](./Examples/Python-ASR-wav2vec2/README.md) |

## Model Enablement

Contains examples for : 

1. Using models, that are not directly supported with AI SDK
2. Debug Quantizatin accuracy loss

|   Type    | SDK   |   Details   |   Link |
|  :---:    |    :---:   |    :---:  |   :---:  |
| Model Conversion Guide | Neural Processing SDK | Model-Accuracy-Mixed-Precision | [ReadMe](./Model-Enablement/Model-Accuracy-Mixed-Precision/README.md)|
| Model Conversion Guide | Neural Processing SDK | Model-Conversion-Layer-Replacement | [ReadMe](./Model-Enablement/Model-Conversion-Layer-Replacement/README.md)|
| Model Conversion Guide | Neural Processing SDK | Model-Conversion-UDO-SELU | [ReadMe](./Model-Enablement/Model-Conversion-UDO-SELU/README.md)|

## Solutions

Contain end-to-end ready-to-run solutions

|   Type     | Solution   |   SDK   |sdk version|   API   | Model   |   ReadMe |  Demo   |
|  :---:     |    :---:   |    :---:  |    :---:  |    :---:  |    :---:  |   :---:  |  :---:  |
|  NLP       | Question Answering       |  Neural Processing SDK | v2.25.0 |Native API | Electra-small     |  [ReadMe](./Solutions/NLPSolution1-QuestionAnswering/README.md) |   [Demo](./Solutions/NLPSolution1-QuestionAnswering/README.md#qa-app-workflow)   |
|  NLP       | Sentiment Analysis       |  Neural Processing SDK | v2.25.0 | Native API | MobileBERT     |  [ReadMe](./Solutions/NLPSolution2-SentimentAnalysis/README.md)  |   [Demo](./Solutions/NLPSolution2-SentimentAnalysis/README.md#sa-app-workflow)   |
|  NLP       | ASR  | Neural Processing SDK | v2.25.0 | Native API | Whisper | [ReadMe](./Solutions/NLPSolution3-AutomaticSpeechRecognition-Whisper/README.md) | [Demo](./Solutions/NLPSolution3-AutomaticSpeechRecognition-Whisper/README.md#Result) |                   
|  Vision    | Object Detection       |  Neural Processing SDK | v2.10.0 |   Java API  | Mobilenet SSD V2    | [ReadMe](./Solutions/VisionSolution1-ObjectDetection/README.md) |   [Demo](./Solutions/VisionSolution1-ObjectDetection/demo/ObjectDetection-Demo.gif)   |
|  Vision    | Object Detection YoloNAS | Neural Processing SDK | v2.25.0 | Native API | YoloNAS| [ReadMe](./Solutions/VisionSolution1-ObjectDetection-YoloNas/README.md)| [Demo](./Solutions/VisionSolution1-ObjectDetection-YoloNas/demo/ObjectDetectYoloNAS.gif)|
|  Vision    | Image Super Resolution       |Neural Processing SDK | v2.25.0 |   Java API | SESR XL    | [ReadMe](./Solutions/VisionSolution2-ImageSuperResolution/README.md) |   [Demo](./Solutions/VisionSolution2-ImageSuperResolution/demo/VisionSolution2-ImageSuperResolution.gif)   |
|  Vision    | Image Enhancement       |Neural Processing SDK | v2.25.0 |  Java API | EnhancedGAN    | [ReadMe](./Solutions/VisionSolution3-ImageEnhancement/README.md)  |   [Demo](./Solutions/VisionSolution3-ImageEnhancement/demo/VisionSolution3-ImageEnhancement.gif)   |
|  Vision    | Pose Estimation |Neural Processing SDK| v2.25.0 | Native API|YoloNAS + HRNet| [ReadMe](./Solutions/VisionSolution4-PoseEstimation/README.md)|[Demo](./Solutions/VisionSolution4-PoseEstimation/demo/PoseDetectionYoloNas.gif)|
|  Vision    | Detection Transformer | Neural Processing SDK | v2.25.0 | Native API | DETR | [ReadMe](./Solutions/VisionSolution1-ObjectDetection-DETR/README.md) | [Demo](./Solutions/VisionSolution1-ObjectDetection-DETR/demo/ObjectDetectDETR.avi)|

## Tools

Contain tools to simplify workflow

|   Tool    | SDK   | Version   |   Details   |   Link |
|  :---:    |    :---:   |    :---:   |    :---:  |   :---:  |
|  PySNPE   | Neural Processing SDK  | - |Python Interface to SDK tools | [ReadMe](./Tools/pysnpe_utils/README.md) |
|  snpe_qnn_docker    | Neural Processing SDK <br>&<br> AI Engine Direct  | 2.25.0+ | Docker container for SDK | [ReadMe](./Tools/snpe_qnn_docker/README.md) |
|  snpe-helper    | Neural Processing SDK  |  - |Python wrapper for C++ API | [ReadMe](./Tools/snpe-helper/README.md) |

## Report Issues

All deliverables were periodically verified on latest Qualcomm AI Stack SDK releases. 
Please report any issues in _issues_ section of GitHub repository. 

Pls write to qidk@qti.qualcomm.com for any questions/suggestions

## Team

Qualcomm Innovators Development Kit (QIDK) software repository is a project maintained by Qualcomm Innovation Center, Inc.

## License 

Please see the [LICENSE](LICENSE) for more details.

###### *Qualcomm Neural Processing SDK, and Qualcomm Innovators Development Kit are products of Qualcomm Technologies, Inc. and/or its subsidiaries. AIMET is a product of Qualcomm Innovation Center, Inc.*
