//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include <jni.h>
#include <string>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <genie/GenieCommon.h>
#include <genie/GenieDialog.h>
#include <chrono>
#include <iostream>
#include <mutex>

// =====================
// Logging
// =====================
#define LOG_TAG "Genie_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// =====================
// Shared State (protected by mutex)
// =====================
static JavaVM* g_jvm = nullptr;                        // JVM to attach threads in callback
static std::mutex g_mu;

static jobject   g_callback_global = nullptr;          // Global ref to Java callback object
static jmethodID g_handleString    = nullptr;          // Method ID: String handleString(String)

static GenieDialogConfig_Handle_t nativeConfigHandle = nullptr;
static GenieDialog_Handle_t       dialogHandle       = nullptr;

static std::string g_config_json;                      // last model config JSON
constexpr const uint32_t maxNumTokens = 200;

// =====================
// Utility: check JNI exceptions (logs + clear)
// =====================
static bool CheckAndClearJniException(JNIEnv* env, const char* where) {
    if (!env) return true; // nothing we can do
    if (env->ExceptionCheck()) {
        LOGE("JNI exception at: %s", where);
        env->ExceptionDescribe();  // Logs stack to logcat
        env->ExceptionClear();     // Clear to continue safely
        return true;
    }
    return false;
}

// =====================
// RAII wrapper for GetStringUTFChars
// =====================
class JniUtfChars {
public:
    JniUtfChars(JNIEnv* env, jstring js)
            : env_(env), js_(js), cstr_(nullptr) {
        if (env_ && js_) cstr_ = env_->GetStringUTFChars(js_, nullptr);
    }
    ~JniUtfChars() {
        if (env_ && js_ && cstr_) env_->ReleaseStringUTFChars(js_, cstr_);
    }
    const char* get() const { return cstr_; }
    bool valid() const { return cstr_ != nullptr; }
private:
    JNIEnv*  env_;
    jstring  js_;
    const char* cstr_;
};

// =====================
// JNI Attach/Detach helpers
// =====================
static JNIEnv* GetEnvForCurrentThread(bool& didAttach) {
    didAttach = false;
    if (!g_jvm) return nullptr;

    JNIEnv* env = nullptr;
    jint res = g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (res == JNI_OK && env) return env;

#if defined(__ANDROID__)
    res = g_jvm->AttachCurrentThread(&env, nullptr);
#else
    res = g_jvm->AttachCurrentThread(reinterpret_cast<void**>(&env), nullptr);
#endif
    if (res != JNI_OK) {
        LOGE("AttachCurrentThread failed: %d", res);
        return nullptr;
    }
    didAttach = true;
    return env;
}

static void DetachIfAttached(bool didAttach) {
    if (didAttach && g_jvm) g_jvm->DetachCurrentThread();
}

// =====================
// ADSP / LD path setup
// =====================
static bool SetAdspLibraryPath(std::string nativeLibPath) {
    nativeLibPath += ";/storage/emulated/0/Android/data/com.example.tts/files/llama_3p2_3b/";
    LOGI("ADSP Lib Path = %s", nativeLibPath.c_str());
    const int a = setenv("ADSP_LIBRARY_PATH", nativeLibPath.c_str(), 1 /*override*/);
    const int b = setenv("LD_LIBRARY_PATH",   nativeLibPath.c_str(), 1 /*override*/);
    if (a != 0 || b != 0) {
        LOGE("Failed to set ADSP/LD envs (setenv returned a=%d, b=%d)", a, b);
        return false;
    }
    return true;
}

// =====================
// Cleanup
// =====================
static void clean_genie() {
    std::lock_guard<std::mutex> lk(g_mu);

    Genie_Status_t st1 = GENIE_STATUS_SUCCESS, st2 = GENIE_STATUS_SUCCESS;

    if (dialogHandle) {
        st1 = GenieDialog_free(dialogHandle);
        dialogHandle = nullptr;
        if (st1 == GENIE_STATUS_SUCCESS) LOGI("Genie dialog freed.");
        else LOGE("Genie dialog free failed, status=%d", st1);
    }

    if (nativeConfigHandle) {
        st2 = GenieDialogConfig_free(nativeConfigHandle);
        nativeConfigHandle = nullptr;
        if (st2 == GENIE_STATUS_SUCCESS) LOGI("Genie config freed.");
        else LOGE("Genie config free failed, status=%d", st2);
    }

    // Clear Java callback global ref safely
    bool didAttach = false;
    JNIEnv* env = GetEnvForCurrentThread(didAttach);
    if (env && g_callback_global) {
        env->DeleteGlobalRef(g_callback_global);
        g_callback_global = nullptr;
        g_handleString    = nullptr;
        LOGI("Deleted callback global ref.");
        CheckAndClearJniException(env, "DeleteGlobalRef");
    }
    DetachIfAttached(didAttach);
}

// =====================
// Initialize (throws on failure)
// =====================
static void initializeGenie(const std::string& nativeJsonStr) {
    std::lock_guard<std::mutex> lk(g_mu);

    // Ensure clean state
    if (dialogHandle) { GenieDialog_free(dialogHandle); dialogHandle = nullptr; }
    if (nativeConfigHandle) { GenieDialogConfig_free(nativeConfigHandle); nativeConfigHandle = nullptr; }

    Genie_Status_t st = GenieDialogConfig_createFromJson(nativeJsonStr.c_str(), &nativeConfigHandle);
    if (st != GENIE_STATUS_SUCCESS || !nativeConfigHandle) {
        throw std::runtime_error("GenieDialogConfig_createFromJson failed (status=" + std::to_string(st) + ")");
    }

    st = GenieDialog_create(nativeConfigHandle, &dialogHandle);
    if (st != GENIE_STATUS_SUCCESS || !dialogHandle) {
        GenieDialogConfig_free(nativeConfigHandle);
        nativeConfigHandle = nullptr;
        throw std::runtime_error("GenieDialog_create failed (status=" + std::to_string(st) + ")");
    }

    st = GenieDialog_setMaxNumTokens(dialogHandle, maxNumTokens);
    if (st != GENIE_STATUS_SUCCESS) {
        GenieDialog_free(dialogHandle); dialogHandle = nullptr;
        GenieDialogConfig_free(nativeConfigHandle); nativeConfigHandle = nullptr;
        throw std::runtime_error("GenieDialog_setMaxNumTokens failed (status=" + std::to_string(st) + ")");
    }
    LOGI("Genie initialized: config+dialog+max_tokens=%u", maxNumTokens);
}

// =====================
// Static callback (attaches to JVM thread, robust JNI checks)
// =====================
static void GenieQueryCallback(const char* response,
                               const GenieDialog_SentenceCode_t /*sentenceCode*/,
                               const void* /*userData*/) {
    bool didAttach = false;
    JNIEnv* env = GetEnvForCurrentThread(didAttach);
    if (!env) {
        LOGE("QueryCallback: JNIEnv is null (attach failed)");
        return;
    }

    // Snapshot callback state under lock
    jobject   callbackObj = nullptr;
    jmethodID methodId    = nullptr;
    {
        std::lock_guard<std::mutex> lk(g_mu);
        callbackObj = g_callback_global;
        methodId    = g_handleString;
    }
    if (!callbackObj || !methodId) {
        LOGE("QueryCallback: callback not set");
        DetachIfAttached(didAttach);
        return;
    }

    jstring jStr = env->NewStringUTF(response ? response : "");
    if (!jStr) {
        LOGE("QueryCallback: NewStringUTF failed (out of memory?)");
        CheckAndClearJniException(env, "NewStringUTF");
        DetachIfAttached(didAttach);
        return;
    }

    env->CallObjectMethod(callbackObj, methodId, jStr);
    if (CheckAndClearJniException(env, "CallObjectMethod(handleString)")) {
        LOGE("QueryCallback: handleString threw a Java exception");
    }
    env->DeleteLocalRef(jStr);
    CheckAndClearJniException(env, "DeleteLocalRef(jStr)");

    DetachIfAttached(didAttach);
}

// =====================
// JNI Exports
// =====================

// Cache JavaVM once
static void EnsureJavaVM(JNIEnv* env) {
    if (g_jvm) return;
    JavaVM* jvm = nullptr;
    if (env->GetJavaVM(&jvm) == JNI_OK && jvm) {
        g_jvm = jvm;
    } else {
        LOGE("Failed to obtain JavaVM from JNIEnv");
    }
}




extern "C"
JNIEXPORT jint JNICALL
Java_com_example_asr_1llm_1tts_MainActivity_free_1genie(JNIEnv* env, jobject /*thiz*/) {
    (void)env; // we attach inside clean_genie if needed
    clean_genie();
    return 0;
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_asr_1llm_1tts_MainActivity_reset_1genie(JNIEnv *env, jobject thiz) {
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
Java_com_example_asr_1llm_1tts_llm_1assistant_Assistant_load_1model(JNIEnv* env, jobject /*thiz*/,
                                                                    jstring json_str,
                                                                    jstring native_dir_path) {
    if (!env || !json_str || !native_dir_path) {
        LOGE("load_model: invalid JNI inputs");
        return -1;
    }

    EnsureJavaVM(env);
    if (!g_jvm) return -1;

    JniUtfChars jsonC(env, json_str);
    JniUtfChars dirC(env, native_dir_path);
    if (!jsonC.valid() || !dirC.valid()) {
        LOGE("load_model: failed to extract strings (GetStringUTFChars returned null)");
        CheckAndClearJniException(env, "GetStringUTFChars(load_model)");
        return -1;
    }

    g_config_json.assign(jsonC.get());
    std::string nativeLibPath(dirC.get());

    if (!SetAdspLibraryPath(nativeLibPath)) {
        LOGE("ADSP/LD path setup failed");
        return -1;
    }
    LOGI("ADSP/LD path setup OK");

    try {
        initializeGenie(g_config_json);
        LOGI("Model loaded successfully.");
        return 0;
    } catch (const std::exception& e) {
        LOGE("initializeGenie failed: %s", e.what());
        clean_genie();
        return -1;
    } catch (...) {
        LOGE("initializeGenie failed: unknown exception");
        clean_genie();
        return -1;
    }
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_asr_1llm_1tts_llm_1assistant_Assistant_genie_1infer(JNIEnv* env, jobject /*thiz*/,
                                                                     jstring text,
                                                                     jobject intermediate_result_callback) {
    if (!env || !text || !intermediate_result_callback) {
        LOGE("genie_infer: invalid JNI inputs");
        return -1;
    }

    // Prepare prompt
    JniUtfChars promptC(env, text);
    if (!promptC.valid()) {
        LOGE("genie_infer: prompt extraction failed");
        CheckAndClearJniException(env, "GetStringUTFChars(genie_infer)");
        return -1;
    }

    // Prepare Java callback (global ref + method ID)
    {
        std::lock_guard<std::mutex> lk(g_mu);

        if (g_callback_global) {
            env->DeleteGlobalRef(g_callback_global);
            g_callback_global = nullptr;
            g_handleString    = nullptr;
            CheckAndClearJniException(env, "DeleteGlobalRef(old callback)");
        }

        g_callback_global = env->NewGlobalRef(intermediate_result_callback);
        if (!g_callback_global) {
            LOGE("genie_infer: NewGlobalRef failed (callback)");
            CheckAndClearJniException(env, "NewGlobalRef(callback)");
            return -1;
        }

        jclass callbackClass = env->GetObjectClass(intermediate_result_callback);
        if (!callbackClass) {
            LOGE("genie_infer: GetObjectClass failed (callback)");
            CheckAndClearJniException(env, "GetObjectClass(callback)");
            env->DeleteGlobalRef(g_callback_global);
            g_callback_global = nullptr;
            return -1;
        }

        g_handleString = env->GetMethodID(callbackClass, "handleString", "(Ljava/lang/String;)Ljava/lang/String;");
        CheckAndClearJniException(env, "GetMethodID(handleString)");
        env->DeleteLocalRef(callbackClass);

        if (!g_handleString) {
            LOGE("genie_infer: handleString(String) not found on callback");
            env->DeleteGlobalRef(g_callback_global);
            g_callback_global = nullptr;
            return -1;
        }
    }

    // Check dialog readiness
    {
        std::lock_guard<std::mutex> lk(g_mu);
        if (!dialogHandle) {
            LOGE("genie_infer: dialog not initialized");
            return -1;
        }
    }

    // Perform inference with timing
    auto start = std::chrono::high_resolution_clock::now();
    Genie_Status_t infer_status = GenieDialog_query(
            dialogHandle,
            promptC.get(),
            GENIE_DIALOG_SENTENCE_COMPLETE,
            GenieQueryCallback,
            nullptr /* userData not required since we use globals */
    );

    // Recovery: free + re-init + retry once
    if (infer_status != GENIE_STATUS_SUCCESS) {
        LOGE("GenieDialog_query failed (status=%d). Attempting recovery…", infer_status);
        try {
            Genie_Status_t reset_status=GenieDialog_reset(dialogHandle);
            if(reset_status==GENIE_STATUS_SUCCESS){
                LOGI("GENIE-Dialog free is successful.");
            } else {
                LOGE("GENIE-Dialog free is failed with status: %d", reset_status);

            }
            LOGI("Rerunning Query");
            infer_status = GenieDialog_query(
                    dialogHandle,
                    promptC.get(),
                    GENIE_DIALOG_SENTENCE_COMPLETE,
                    GenieQueryCallback,
                    nullptr
            );
        } catch (const std::exception& e) {
            LOGE("Recovery initializeGenie failed: %s", e.what());
        } catch (...) {
            LOGE("Recovery initializeGenie failed: unknown exception");
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    LOGI("Inference took %lld ms", static_cast<long long>(durationMs));

    if (infer_status == GENIE_STATUS_SUCCESS) {
        LOGI("Inference successful.");
        return 0;
    } else {
        LOGE("Inference failed with status: %d", infer_status);
        return static_cast<jint>(infer_status);
    }
}