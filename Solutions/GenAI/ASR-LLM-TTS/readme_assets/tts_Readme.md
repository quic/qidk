# Prerequisite
1. Download [VoiceAI-TTS-1.1.0.0](https://qpm.qualcomm.com/#/main/tools/details/VoiceAI_TTS) from QPM.
    ```
    qpm-cli --login <username>
    qpm-cli --install VoiceAI_TTS
    ```
1. Generate melo tts model via Notebook
2. Download suitable QNN package via Qualcomm Package Manager:
   Download the Qualcomm AI Runtime SDK (QNN SDK) Linux version `2.37.0.250724` from https://qpm.qualcomm.com/#/main/tools/details/Qualcomm_AI_Runtime_SDK
   Follow the instructions on the website to use the qpm-cli tool to extract the qik file `Qualcomm_AI_Runtime_SDK.2.37.0.250724.Linux-AnyCPU.qik`.
   Copy the extracted `2.37.0.250724` folder to the working_dir directory.
   ```
   qpm-cli --login <username>
   qpm-cli --catalog-refresh
   qpm-cli --license-activate Qualcomm_AI_Runtime_SDK
   qpm-cli --extract Qualcomm_AI_Runtime_SDK.2.37.0.250724.Linux-AnyCPU.qik
   ```

# Android app preparation
1. Create `libs` directory in `./sample_app/app/`
2. Copy `tts-sdk.jar` in `melo_sdk/libs/npu/rpc_libraries/android/` to `./sample_app/app/libs/`.
3. Copy tts native libs `libtts_jni.so` and `libtts.so` in `melo_sdk/libs/npu/rpc_libraries/android/arm64-v8a/` to `./sample_app/app/src/main/jniLibs/arm64-v8a/`.
4. Check and update the list of model name in TTSSample project's `app\src\main\res\values\languages.xml` file which should be same as the one generated via Notebook
   ```
   <string-array name="language_models">
       <item>melo_en.64_bit.qnn_v2.33.0.qnn</item>
       <item>melo_zh.64_bit.qnn_v2.33.0.qnn</item>
       <item>melo_es.64_bit.qnn_v2.33.0.qnn</item>
   </string-array>
   ```

# Setup device

1. Push generated tts models to the device `/storage/emulated/0/Android/data/com.example.tts/files/tts/` directory, take English tts mode melo_en.64_bit.qnn_v2.33.0.qnn for example

    ```bash
    adb push melo_en.64_bit.qnn_v2.33.0.qnn /storage/emulated/0/Android/data/com.example.tts/files/tts/
    ```
2. Push tts skel lib to the device `/storage/emulated/0/Android/data/com.example.tts/files/tts/` directory

    ```bash
    adb push melo_sdk/libs/npu/rpc_libraries/cdsp/libtts_impl_skel.so /storage/emulated/0/Android/data/com.example.tts/files/tts
    ```
 3. Push the qnn-2.40(tested) libs to the device.
    - libQnnHtpVXX.so (such as libQnnHtpV81.so for SM8850, libQnnHtpV79.so for SM8750)
    - libQnnSystem.so
    #### SoC → Version Mapping

        SM8850: V81  
        SM8750: V79

        ```bash

        adb push ${QNN Package}/lib/hexagon-v79/unsigned/libQnnSystem.so /storage/emulated/0/Android/data/com.example.tts/files/tts/
        adb push ${QNN Package}/lib/hexagon-v79/unsigned/libQnnHtpV79.so /storage/emulated/0/Android/data/com.example.tts/files/tts/
        ```
		
   After these steps done, the device folder `/storage/emulated/0/Android/data/com.example.tts/files/tts/` should contain following models:
   > - melo_xx.64_bit.qnn_v2.33.0.qnn (such as melo_en.64_bit.qnn_v2.33.0.qnn)
   > - libtts_impl_skel.so
   > - libQnnSystem.so
   > - libQnnHtpV79.so (such as libQnnHtpV81.so for SM8850, libQnnHtpV79.so for SM8750)



