# Prerequisite
- This pipeline supports generating models with voice-ai Notebook or downloading models from AI-HUB.
-  [Voice-AI-ASR-2.1.0.0](https://qpm.qualcomm.com/#/main/tools/details/VoiceAI_ASR)
    ```
    qpm-cli --login <username>
    qpm-cli --install VoiceAI_ASR
    ```
## Case 1 Prepare whisper model with notebook
1. Generate whisper model via Notebook
2. Download suitable QNN package via Qualcomm Package Manager:
  Download the Qualcomm AI Runtime SDK (QNN SDK) Linux version 2.37.0.250724 from https://qpm.qualcomm.com/#/main/tools/details/Qualcomm_AI_Runtime_SDK
  Follow the instructions on the website to use the qpm-cli tool to extract the qik file `Qualcomm_AI_Runtime_SDK.2.37.0.250724.Linux-AnyCPU.qik`.
  ```
  qpm-cli --login <username>
  qpm-cli --license-activate Qualcomm_AI_Runtime_SDK
  qpm-cli --extract Qualcomm_AI_Runtime_SDK.2.37.0.250724.Linux-AnyCPU.qik
  ```

## Case 2 Prepare whisper model with AI-Hub
The following steps guide you through downloading pre-trained Whisper models from AI-Hub and preparing them for use with your application.
Take model of Whisper-Small and device of Snapdragon® 8 Elite Mobile as example.
1. Download whisper models via AI-Hub https://aihub.qualcomm.com/models/whisper_small?searchTerm=whisper
  a. Click "Download Model"
  b. Choose runtime "Qualcomm AI Runtime"
  c. Choose device "Snapdragon® 8 Elite Mobile"
  d. Download HfWhisperDecoder and HfWhisperEncoder respectively
  e. Rename the downloaded models to decoder_model_htp.bin and encoder_model_htp.bin
2. Get QNN version of the target models and device
  a. Choose device "Snapdragon® 8 Elite Mobile"
  b. Select "TorchScript-> Qualcomm AI Runtime"
  c. Click "See more metrics" to get QNN(QAIRT) version
3. Download suitable QNN package(get from step 2) via Qualcomm Package Manager:
  Take v2.37.0.250724 as example
  Download the Qualcomm AI Runtime SDK (QNN SDK) Linux version 2.37.0.250724 from https://qpm.qualcomm.com/#/main/tools/details/Qualcomm_AI_Runtime_SDK
  Follow the instructions on the website to use the qpm-cli tool to extract the qik file `Qualcomm_AI_Runtime_SDK.2.37.0.250724.Linux-AnyCPU.qik`.
  ```
  qpm-cli --login <username>
  qpm-cli --license-activate Qualcomm_AI_Runtime_SDK
  qpm-cli --extract Qualcomm_AI_Runtime_SDK.2.37.0.250724.Linux-AnyCPU.qik
  ```
4. Generate vocab.bin via Notebook notebook/whisper/npu/whisper_vocab/

Finally, Whisper models contain below bin:
  1. encoder_model_htp.bin
  2. decoder_model_htp.bin
  3. vocab.bin

## Build android app
1. Create "libs" directory in "./sample_app/app/" and also create "jniLibs/arm64-v8a" directory in "./sample_app/app/src/main".
2. Copy "whisper_sdk/libs/npu/rpc_libraries/android/whisper_all_quantized/whisper-sdk.jar" to "./sample_app/app/libs/".
3. Copy "whisper_sdk/libs/npu/rpc_libraries/android/whisper_all_quantized/arm64-v8a" to "./sample_app/app/src/main/jniLibs/".
4. Copy the VAD model "/whisper_sdk/libs/npu/rpc_libraries/assets/arm64-v8a_android/libnnvad_model.so" to  "./sample_app/app/libs/".
5. Copy {QNN Package}/lib/hexagon-vxx/unsigned/libQnnCpu.so to "./sample_app/app/src/main/jniLibs/".


## Setup on-device model
1. Push Whisper models to the device
    ```
    adb push decoder_model_htp.bin /storage/emulated/0/Android/data/com.example.tts/files/whisper_small/
    adb push encoder_model_htp.bin /storage/emulated/0/Android/data/com.example.tts/files/whisper_small/
    adb push vocab.bin             /storage/emulated/0/Android/data/com.example.tts/files/whisper_small/
    ```

2. Push below QNN-2.40->(last tested) libs to the device
    ```
    adb push ${QNN Package}/lib/hexagon-vxx/unsigned/libQnnHtpVxxSkel.so /storage/emulated/0/Android/data/com.example.tts/files/whisper_small/
    ```
    
    #### SoC → Version Mapping

    SM8850: V81  
    SM8750: V79
    ```
