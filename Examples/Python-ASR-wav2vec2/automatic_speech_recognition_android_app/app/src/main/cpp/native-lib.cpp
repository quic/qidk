//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include <jni.h>
#include <string>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "hpp/inference.h"
#include <jni.h>
#include <jni.h>
#include <jni.h>

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_automatic_1speech_1recognition_ASRHelper_queryRuntimes(JNIEnv *env, jobject thiz,
                                                                        jstring native_dir_path) {
    const char *cstr = env->GetStringUTFChars(native_dir_path, NULL);
    env->ReleaseStringUTFChars(native_dir_path, cstr);

    std::string runT_Status;
    std::string nativeLibPath = std::string(cstr);


    if (!SetAdspLibraryPath(nativeLibPath)) {
        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "Failed to set ADSP Library Path\n");

        runT_Status += "\nFailed to set ADSP Library Path\nTerminating";
        return env->NewStringUTF(runT_Status.c_str());
    }


    // ====================================================================================== //

    runT_Status = "Querying Runtimes : \n\n";
    // DSP unsignedPD check
    if (!zdl::SNPE::SNPEFactory::isRuntimeAvailable(zdl::DlSystem::Runtime_t::DSP,zdl::DlSystem::RuntimeCheckOption_t::UNSIGNEDPD_CHECK)) {
        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "UnsignedPD DSP runtime : Absent\n");
        runT_Status += "UnsignedPD DSP runtime : Absent\n";
    }
    else {
        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "UnsignedPD DSP runtime : Present\n");
        runT_Status += "UnsignedPD DSP runtime : Present\n";
    }
    // DSP signedPD check
    if (!zdl::SNPE::SNPEFactory::isRuntimeAvailable(zdl::DlSystem::Runtime_t::DSP)) {
        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "DSP runtime : Absent\n");
        runT_Status += "DSP runtime : Absent\n";
    }
    else {
        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "DSP runtime : Present\n");
        runT_Status += "DSP runtime : Present\n";
    }
    // GPU check
    if (!zdl::SNPE::SNPEFactory::isRuntimeAvailable(zdl::DlSystem::Runtime_t::GPU)) {
        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "GPU runtime : Absent\n");
        runT_Status += "GPU runtime : Absent\n";
    }
    else {
        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "GPU runtime : Present\n");
        runT_Status += "GPU runtime : Present\n";
    }
    // CPU check
    if (!zdl::SNPE::SNPEFactory::isRuntimeAvailable(zdl::DlSystem::Runtime_t::CPU)) {
        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "CPU runtime : Absent\n");
        runT_Status += "CPU runtime : Absent\n";
    }
    else {
        __android_log_print(ANDROID_LOG_INFO, "SNPE ", "CPU runtime : Present\n");
        runT_Status += "CPU runtime : Present\n";
    }

    return env->NewStringUTF(runT_Status.c_str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_automatic_1speech_1recognition_ASRHelper_initSNPE(JNIEnv *env, jobject thiz,
                                                                   jobject asset_manager) {
    LOGI("ASRhelper: Reading SNPE DLC ...");
    std::string result;

    AAssetManager* mgr = AAssetManager_fromJava(env, asset_manager);
    AAsset* asset = AAssetManager_open(mgr, "wav2vec2_w8a16.dlc", AASSET_MODE_UNKNOWN);
    if (NULL == asset) {
        LOGE("ASRHelper: Failed to load ASSET, needed to load DLC\n");
        result = "Failed to load ASSET, needed to load DLC\n";
        return env->NewStringUTF(result.c_str());
    }
    long dlc_size = AAsset_getLength(asset);
    LOGI("ASRHelper: DLC Size = %ld MB\n", dlc_size / (1024*1024));

    char* dlc_buffer = (char*) malloc(sizeof(char) * dlc_size);
    AAsset_read(asset, dlc_buffer, dlc_size);


    result += build_network(reinterpret_cast<const uint8_t *>(dlc_buffer), dlc_size);

    return env->NewStringUTF(result.c_str());
}
extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_automatic_1speech_1recognition_ASRHelper_inferSNPE(JNIEnv *env, jobject thiz, jstring runtime,
                                                                    jfloatArray input_ids,
                                                                    jfloatArray logits) {
    std::string return_msg;
    jfloat * inp_id_array;
    jfloat * logit_array;


    const char *accl_name = env->GetStringUTFChars(runtime, NULL);
    env->ReleaseStringUTFChars(runtime, accl_name);
    std::string backend = std::string(accl_name);


    inp_id_array = env->GetFloatArrayElements(input_ids, NULL);
    logit_array=env->GetFloatArrayElements(logits,NULL);

//
//
//    // do some exception checking
    if (inp_id_array == NULL) {
        return_msg += "0.0 0.0 Err: Invalid input_id_array/attn_mask_arr/arr_sizes\n";
        return env->NewStringUTF(return_msg.c_str()); /* exception occurred */
    }

//    //Creating input vector and output vector to pass to the DLC Model
    std::vector<float *> inputVec {inp_id_array};
    std::vector<float *> outputVec {logit_array};

    std::size_t size = sizeof(inputVec[0][0]);
    LOGI("ASRHelper: input_vec size %d",size);
    for (int idx=0; idx <4; ++idx) {
        LOGI("ASRHelper native-lib: %d -> %f", idx,inputVec[0][idx]);
    }

    return_msg = execute_net(inputVec,40000,outputVec, backend);
    for ( int index = 0; index < 3968; index++ ) {
//        LOGI("out[%d] = %f ", index, outputVec[1][index]);
        logit_array[index] = outputVec[0][index];
    }
    for ( int s = 0; s < 3; s++ )
        LOGI("ASRHelper: native-lib: out[0][%d] = %f ",s, logit_array[s]);

//    // ===================================================================== //
//    // release the memory so java can have it again
    env->ReleaseFloatArrayElements(input_ids, inp_id_array, 0);
    env->ReleaseFloatArrayElements(logits,logit_array, 0);
//
//
    return env->NewStringUTF(return_msg.c_str());

}


