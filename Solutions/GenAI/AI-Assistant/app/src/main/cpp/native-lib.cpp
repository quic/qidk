//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include <jni.h>
#include <string>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <jni.h>
#include <android/log.h>
#include <genie/GenieCommon.h>
#include <genie/GenieDialog.h>
#include <chrono>



//Setting the TAGs
#define LOG_TAG "Genie_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)



GenieDialogConfig_Handle_t nativeConfigHandle;
GenieDialog_Handle_t dialogHandle;



static JNIEnv* g_env= nullptr;

static jobject g_intermediate_result_callback= nullptr;
static jmethodID g_handleString= nullptr;


//Setting the ADSP path
bool SetAdspLibraryPath(std::string nativeLibPath) {
    nativeLibPath += ";/data/local/tmp";
    LOGI("ADSP Lib Path = %s", nativeLibPath.c_str());
    const int a = setenv("ADSP_LIBRARY_PATH", nativeLibPath.c_str(), 1 /*override*/);
    const int b = setenv("LD_LIBRARY_PATH",   nativeLibPath.c_str(), 1 /*override*/);

    if (a != 0 || b != 0) {
        LOGE("Failed to set ADSP/LD envs (setenv returned a=%d, b=%d)", a, b);
        return false;
    }
    return true;
}

//Loading the model
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ai_1assistant_1v1_MainActivity_load_1model(JNIEnv *env, jobject thiz,
                                                            jstring json_str,
                                                            jstring native_dir_path) {


    const char* nativeJsonStr = env->GetStringUTFChars(json_str, 0);
    const char *cstr = env->GetStringUTFChars(native_dir_path, nullptr);
    env->ReleaseStringUTFChars(native_dir_path, cstr);

    std::string runT_Status;
    std::string nativeLibPath = std::string(cstr);


    //Digesting Config to GENIE-Compatible Config file
    Genie_Status_t status = GenieDialogConfig_createFromJson(nativeJsonStr, &nativeConfigHandle);

    //If it's not successful,then simply return, this might be the case that artifacts paths has not set properly
    if(status==GENIE_STATUS_SUCCESS){
        LOGI("Configuration handling successful.");
    } else {
        LOGE("Configuration handling failed with status: %d", status);
        return status;
    }

    //Setting the Necessary Library path to be used from the device
    if (!SetAdspLibraryPath(nativeLibPath)) {
        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "Failed to set ADSP Library Path\n");
        LOGE("ADSP Path set is failed: %d", -1);
        return -1;
    }
    else{
        LOGI("ADSP Path set is Successful.");
    }

    //Creating Dialog from the digested config file
    Genie_Status_t dialog_status = GenieDialog_create(nativeConfigHandle, &dialogHandle);
    if(dialog_status==GENIE_STATUS_SUCCESS){
        LOGI("Dialog creation is  successful.");
    } else {
        LOGE("Dialog  handling failed with status: %d", dialog_status);
        return dialog_status;
    }

    //Now model loading is successful
    //User can do the inference


    return 0;
}
//Inference function
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ai_1assistant_1v1_Assistant_genie_1infer(JNIEnv *env, jobject thiz, jstring text,jint chat_count,
                                                          jobject intermediate_result_callback) {


    const char *rawString = env->GetStringUTFChars(text, 0);
    const char *str(rawString);
    env->ReleaseStringUTFChars(text, rawString);

    jclass callbackClass = env->GetObjectClass(intermediate_result_callback);
    g_env=env;
    g_intermediate_result_callback=intermediate_result_callback;
    // Method ID
    jmethodID handleString = env->GetMethodID(callbackClass, "handleString", "(Ljava/lang/String;)Ljava/lang/String;");

//    GenieDialog_reset(dialogHandle);

    g_handleString=handleString;


    // Initialize the sentence code
    GenieDialog_SentenceCode_t code = GENIE_DIALOG_SENTENCE_COMPLETE;


    //Callback function to get the result from GENIE
    GenieDialog_QueryCallback_t callback = [](const char* response,
                                              const GenieDialog_SentenceCode_t sentenceCode,
                                              const void* userData) -> void {

        jstring jInputString = g_env->NewStringUTF(response);
        jstring jResponse = (jstring) g_env->CallObjectMethod(g_intermediate_result_callback, g_handleString, jInputString);
    };

    std::string model_response;
    //Calling the genie-query function to get the result
    Genie_Status_t infer_status= GenieDialog_query(dialogHandle,str,code,callback, &model_response);

    LOGI("Model Response: %s  Chat Count: %d",model_response.c_str(),chat_count);

    if (chat_count>1) {
        // If model response is empty, reset dialog to re-initiate dialog.
        // During local testing, we found that in certain cases,
        // model response bails out after few iterations during chat.
        // If that happens, just reset Dialog handle to continue the chat.

        if (GENIE_STATUS_SUCCESS != GenieDialog_reset(dialogHandle)) {
            throw std::runtime_error("Failed to reset Genie Dialog.");
        }
        LOGI("Reset is successful, Chat Count:%d",chat_count);
    }

    if(infer_status==GENIE_STATUS_SUCCESS){
        LOGI("Inference is successful.");
    }
    else {
        LOGE("Inference failed with status: %d", infer_status);
    }

    return 0;
}




extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ai_1assistant_1v1_MainActivity_free_1genie(JNIEnv *env, jobject thiz) {
    // TODO: implement free_genie()

    Genie_Status_t free_status=GenieDialog_free(dialogHandle);
    free_status=GenieDialogConfig_free(nativeConfigHandle);

    if(free_status==GENIE_STATUS_SUCCESS){
        LOGI("GENIE-Dialog free is successful.");
    } else {
        LOGE("GENIE-Dialog free is failed with status: %d", free_status);

    }
    return free_status;
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ai_1assistant_1v1_ChatActivity_reset_1genie(JNIEnv *env, jobject thiz) {
    // TODO: implement reset_genie()
    Genie_Status_t reset_status=GenieDialog_reset(dialogHandle);

    if(reset_status==GENIE_STATUS_SUCCESS){
        LOGI("GENIE-Dialog free is successful.");
    } else {
        LOGE("GENIE-Dialog free is failed with status: %d", reset_status);

    }
    return reset_status;
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_ai_1assistant_1v1_Assistant_reset_1genie(JNIEnv *env, jobject thiz) {
    Genie_Status_t reset_status=GenieDialog_reset(dialogHandle);

    if(reset_status==GENIE_STATUS_SUCCESS){
        LOGI("GENIE-Dialog free is successful.");
    } else {
        LOGE("GENIE-Dialog free is failed with status: %d", reset_status);

    }
    return reset_status;
}