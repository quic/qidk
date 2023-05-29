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

## Examples

Contain examples to use features of above SDKs

|   Type    | SDK   |   Details   |   Link |
|  :---:    |    :---:   |    :---:  |   :---:  |
|  Model    | QNN   |  Model - EnhancementGAN | [ReadMe](./Examples/QNN-Model-Example-EnhancementGAN/README.md) |
|  Model    | QNN   |  Model - SESR | [ReadMe](./Examples/QNN-Model-Example-SESR/README.md) |

## Solutions

Contain end-to-end ready-to-run solutions

|   Type    | Solution   |   Model   |   ReadMe |  Demo   |
|  :---:     |    :---:   |    :---:  |   :---:  |  :---:  |
|  NLP       | Question Answering       |  Electra-small     |  [ReadMe](./Solutions/NLPSolution1-QuestionAnswering/README.md) |   [Demo](./Solutions/NLPSolution1-QuestionAnswering/README.md#qa-app-workflow)   |
|  NLP       | Sentiment Analysis       |  MobileBERT     |  [ReadMe](./Solutions/NLPSolution2-SentimentAnalysis/README.md)  |   [Demo](./Solutions/NLPSolution2-SentimentAnalysis/README.md#sa-app-workflow)   |
|  Vision    | Object Detection       |   Mobilenet SSD V2    | [ReadMe](./Solutions/VisionSolution1-ObjectDetection/README.md) |   [Demo](./Solutions/VisionSolution1-ObjectDetection/demo/ObjectDetection-Demo.gif)   |
|  Vision    | Image Super Resolution       |   SESR XL    | [ReadMe](./Solutions/VisionSolution2-ImageSuperResolution/README.md) |   [Demo](./Solutions/VisionSolution2-ImageSuperResolution/demo/VisionSolution2-ImageSuperResolution.gif)   |
|  Vision    | Image Enhancement       |   EnhancedGAN    | [ReadMe](./Solutions/VisionSolution3-ImageEnhancement/README.md)  |   [Demo](./Solutions/VisionSolution3-ImageEnhancement/demo/VisionSolution3-ImageEnhancement.gif)   |

## Tools

Contain tools to simplify workflow

|   Tool    | SDK   |   Details   |   Link |
|  :---:    |    :---:   |    :---:  |   :---:  |
|  PySNPE   | SNPE   |  Python Interface to SNPE tools | [ReadMe](./Tools/pysnpe_utils/README.md) |
|  snpe-docker    | SNPE   |  Docker container for SNPE | [ReadMe](./Tools/snpe-docker/README.md) |

Pls write to qidk@qti.qualcomm.com for any questions/suggestions

###### *Qualcomm Neural Processing SDK, and Qualcomm Innovators Development Kit are products of Qualcomm Technologies, Inc. and/or its subsidiaries. AIMET is a product of Qualcomm Innovation Center, Inc.*
