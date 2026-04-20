# LLM Generation

## Prerequisite
1. llm model (We tested using llama3.2-3B model)
2. QAIRT


## Model Generation part

### Case 1: Download model from AIHUB
- Follow this page to generate the optimized LLM(llama-3.2-3B) model [AIHUB](https://github.com/quic/ai-hub-apps/tree/main/tutorials/llm_on_genie).


### Case 2: Generate model from QPM-Notebooks

- You can run generate the llama-3.2-3B-SSD(Self Speculative Decoded) model from [QPM-Notebook](https://qpm.qualcomm.com/#/main/tools/details/Tutorial_for_Llama3p2_3B_Instruct_IoT). 

    ```bash
    qpm-cli --login <username>
    qpm-cli --license-activate Tutorial_for_Llama3p2_3B_Instruct_IoT
    qpm-cli --extract Tutorial_for_Llama3p2_3B_Instruct_IoT (or)
    qpm-cli --extract <full path to downloaded .qik file>

    ```
## Setup android app

```bash
mkdir -p ./sample_app/app/src/main/jniLibs/arm64-v8a

cp <QNN_SDK_ROOT>/libs/aarch64-android/libGenie.so \
   <QNN_SDK_ROOT>/libs/aarch64-android/libQnnHtp.so \
   <QNN_SDK_ROOT>/libs/aarch64-android/libQnnHtpVXXStub.so \
   <QNN_SDK_ROOT>/libs/aarch64-android/libQnnSystem.so \
   ./sample_app/app/src/main/jniLibs/arm64-v8a/

```

Copy Genie header files.

```bash
mkdir -p ./sample_app/app/src/main/jniLibs/arm64-v8a

cp <QNN_SDK_ROOT>/include/Genie/* ./sample_app/app/src/main/cpp/genie/
```

### Target directory after coping
```text
sample_app/
└── app/
    └── src/
        └── main/
            └── jniLibs/
                └── arm64-v8a/
                    ├── libGenie.so
                    ├── libQnnHtp.so
                    ├── libQnnHtpVXXStub.so
                    └── libQnnSystem.so

```
## Setup device
-  Now create a folder <llama_3p2_3b> and  push the models, tokenizers, htp_backend_ext_config.json file there. You can get more information from [aihub_llm_on_genie_link](https://github.com/quic/ai-hub-apps/tree/main/tutorials/llm_on_genie).
- Copy  "<QNN_SDK_ROOT>/libs/hexagon-vXX/unsigned/*" libs to the <llama_3p2_3b>.
    #### SoC → Version Mapping

        SM8850: V81  
        SM8750: V79  

- Copy "<QNN_SDK_ROOT>/libs/aarch64-android/*" to the <llama_3p2_3b>

- Now push the artifacts to the device

        adb push <llama_3p2_3b> /storage/emulated/0/Android/data/com.example.tts/files
        
        