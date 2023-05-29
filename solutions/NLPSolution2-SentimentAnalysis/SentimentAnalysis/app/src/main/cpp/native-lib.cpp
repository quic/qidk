#include <jni.h>
#include <string>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "hpp/inference.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_qualcomm_qti_sentimentanalysis_MainActivity_queryRuntimes(
        JNIEnv* env,
        jobject /* this */,
        jstring native_dir_path) {
    const char *cstr = env->GetStringUTFChars(native_dir_path, NULL);
    env->ReleaseStringUTFChars(native_dir_path, cstr);

    std::string runT_Status;
    std::string nativeLibPath = std::string(cstr);

//    runT_Status += "\nLibs Path : " + nativeLibPath + "\n";

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
Java_com_qualcomm_qti_sentimentanalysis_MainActivity_initSNPE(JNIEnv *env, jobject thiz,
                                                              jobject asset_manager) {
    LOGI("Reading SNPE DLC ...");
    std::string result;

    AAssetManager* mgr = AAssetManager_fromJava(env, asset_manager);
    AAsset* asset = AAssetManager_open(mgr, "mobilebert_sst2_cached.dlc", AASSET_MODE_UNKNOWN);
    if (NULL == asset) {
        LOGE("Failed to load ASSET, needed to load DLC\n");
        result = "Failed to load ASSET, needed to load DLC\n";
        return env->NewStringUTF(result.c_str());
    }
    long dlc_size = AAsset_getLength(asset);
    LOGI("DLC Size = %ld MB\n", dlc_size / (1024*1024));
//    result += "DLC Size = " + std::to_string(dlc_size);
    char* dlc_buffer = (char*) malloc(sizeof(char) * dlc_size);
    AAsset_read(asset, dlc_buffer, dlc_size);

    result += "\n\nBuilding Model DLC Network:\n";
    result += build_network(reinterpret_cast<const uint8_t *>(dlc_buffer), dlc_size);

    return env->NewStringUTF(result.c_str());
}
extern "C"
JNIEXPORT jstring JNICALL
Java_com_qualcomm_qti_sentimentanalysis_MainActivity_inferSNPE(JNIEnv *env, jobject thiz,
                                                               jstring runtime, jfloatArray input_ids,
                                                               jfloatArray attn_masks,
                                                               jintArray arraySizes) {
    std::string return_msg;
    jfloat * inp_id_array;
    jfloat * mask_array;
    jint * arrayLengths;

    const char *accl_name = env->GetStringUTFChars(runtime, NULL);
    env->ReleaseStringUTFChars(runtime, accl_name);
    std::string backend = std::string(accl_name);

    // get a pointer to the array
    inp_id_array = env->GetFloatArrayElements(input_ids, NULL);
    mask_array = env->GetFloatArrayElements(attn_masks, NULL);
    arrayLengths = env->GetIntArrayElements(arraySizes, NULL);

    // do some exception checking
    if (inp_id_array == NULL || mask_array == NULL || arrayLengths[0] != arrayLengths[1]) {
        return_msg += "0 0 Err: Invalid input_id_array/attn_mask_arr/arr_sizes\n";
        return env->NewStringUTF(return_msg.c_str()); /* exception occurred */
    }

    std::vector<float *> inputVec { inp_id_array, mask_array };

    return_msg = execute_net(inputVec, arrayLengths, backend);
    LOGI("SNPE JNI : %s", return_msg.c_str());

    // ===================================================================== //
    // release the memory so java can have it again
    env->ReleaseFloatArrayElements(input_ids, inp_id_array, 0);
    env->ReleaseFloatArrayElements(attn_masks, mask_array, 0);
    env->ReleaseIntArrayElements(arraySizes, arrayLengths, 0);

    return env->NewStringUTF(return_msg.c_str());
}