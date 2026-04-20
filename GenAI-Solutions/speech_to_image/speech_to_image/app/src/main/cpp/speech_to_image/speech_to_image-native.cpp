//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include <jni.h>
#include "AndroidLogger.hpp"
#include <cstdlib>
#include <vector>
#include <string>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "QnnSpeechToImage.hpp"
#include <dlfcn.h>
#include "SpeechToImage.hpp"
#include <fstream>
#include <filesystem>

std::string jstring_to_stdstring(JNIEnv* env, jstring str);
std::vector<int32_t> jlongArray_to_vector(JNIEnv* env, jlongArray jArray);

typedef const char* (*TokenizeFunc)(const char*, const char*);
TokenizeFunc _tokenize;
char* tokenizer_path;
void* libtokenizer_handle;
TokenizeFunc _detokenize;
char* detokenizer_path;
void* libdetokenizer_handle;

extern "C" {
  JNIEXPORT jint JNICALL
  Java_com_qualcomm_qti_speech_1to_1image_SpeechToImageNativeJNI_initializeApp(JNIEnv *env, jobject thiz, jstring cacheDir) {
    std::string cacheDirStr = jstring_to_stdstring(env, cacheDir) + "/";

    redirect_cout_cerr_to_logcat();

    return initialize(cacheDirStr);
  }

  JNIEXPORT void JNICALL
  Java_com_qualcomm_qti_speech_1to_1image_SpeechToImageNativeJNI_runStableDiffusion(JNIEnv *env, jobject thiz, jlongArray cond_tokens, jstring file_name) {
    stop_stable_diffusion = false;
    run_stable_diffusion(jlongArray_to_vector(env, cond_tokens), jstring_to_stdstring(env, file_name));
  }

  JNIEXPORT jint JNICALL
  Java_com_qualcomm_qti_speech_1to_1image_SpeechToImageNativeJNI_freeApp(JNIEnv *env, jobject thiz) {
    return free_app();
  }

  JNIEXPORT jint JNICALL
  Java_com_qualcomm_qti_speech_1to_1image_SpeechToImageNativeJNI_initializeTokenizer(JNIEnv *env, jobject thiz, jstring path) {
    tokenizer_path = strdup(jstring_to_stdstring(env, path).c_str());

    libtokenizer_handle = dlopen("libtokenizer.so", RTLD_LAZY);
    if (!libtokenizer_handle) {
      LOGE("Error in loading libtokenizer.so: %s", dlerror());
      return 1;
    }

    _tokenize = (TokenizeFunc) dlsym(libtokenizer_handle, "tokenize");
    if (!_tokenize) {
      LOGE("Error in function tokenize dynamic loading: %s", dlerror());
      dlclose(libtokenizer_handle);
      return 2;
    }

    return 0;
  }

JNIEXPORT jint JNICALL
Java_com_qualcomm_qti_speech_1to_1image_SpeechToImageNativeJNI_initializeDetokenizer(JNIEnv *env, jobject thiz, jstring path) {
  detokenizer_path = strdup(jstring_to_stdstring(env, path).c_str());

  libdetokenizer_handle = dlopen("libdetokenizer.so", RTLD_LAZY);
  if (!libdetokenizer_handle) {
    LOGE("Error in loading libdetokenizer.so: %s", dlerror());
    return 1;
  }

  _detokenize = (TokenizeFunc) dlsym(libdetokenizer_handle, "detokenize");
  if (!_detokenize) {
    LOGE("Error in function tokenize dynamic loading: %s", dlerror());
    dlclose(libdetokenizer_handle);
    return 2;
  }

  return 0;
}

  JNIEXPORT jstring JNICALL
  Java_com_qualcomm_qti_speech_1to_1image_SpeechToImageNativeJNI_tokenize(JNIEnv *env, jobject thiz, jstring text) {
    if (!_tokenize) return (*env).NewStringUTF("");

    // Convert jstring to std::string
    std::string text_stdstr = jstring_to_stdstring(env, text);
    const char* text_cstr = text_stdstr.c_str();

    // Tokenize the text
    const char* result = _tokenize(text_cstr, tokenizer_path);

    // Return the result as a new jstring
    return (*env).NewStringUTF(result);
  }

  JNIEXPORT void JNICALL
  Java_com_qualcomm_qti_speech_1to_1image_SpeechToImageNativeJNI_freeTokenizer(JNIEnv *env, jobject thiz) {
    dlclose(libtokenizer_handle);
    _tokenize = nullptr;
  }

  JNIEXPORT jstring JNICALL
  Java_com_qualcomm_qti_speech_1to_1image_SpeechToImageNativeJNI_detokenize(JNIEnv *env, jobject thiz,
                                                                            jstring ids) {
    if (!_detokenize) return (*env).NewStringUTF("");

    // Convert jstring to std::string
    std::string ids_stdstr = jstring_to_stdstring(env, ids);
    const char* ids_cstr = ids_stdstr.c_str();

    // Tokenize the text
    const char* result = _detokenize(ids_cstr, detokenizer_path);

    // Return the result as a new jstring
    return (*env).NewStringUTF(result);
  }

  JNIEXPORT jstring JNICALL
  Java_com_qualcomm_qti_speech_1to_1image_SpeechToImageNativeJNI_runWhisper(JNIEnv *env, jobject thiz,
                                                                            jstring audio_path) {
    return (*env).NewStringUTF(run_whisper(jstring_to_stdstring(env, audio_path)).c_str());
  }

  JNIEXPORT void JNICALL
  Java_com_qualcomm_qti_speech_1to_1image_SpeechToImageNativeJNI_stopStableDiffusion(JNIEnv *env, jobject thiz) {
    stop_stable_diffusion = true;
  }
}

std::vector<int32_t> jlongArray_to_vector(JNIEnv* env, jlongArray jArray) {
  jsize length = env->GetArrayLength(jArray);

  jlong* elements = env->GetLongArrayElements(jArray, nullptr);

  std::vector<int32_t> result;
  result.reserve(length);

  for (jsize i = 0; i < length; ++i) {
    result.push_back(static_cast<int32_t>(elements[i]));
  }

  env->ReleaseLongArrayElements(jArray, elements, 0);

  return result;
}

std::string jstring_to_stdstring(JNIEnv* env, jstring str){
  const char *cstr = env->GetStringUTFChars(str, NULL);
  env->ReleaseStringUTFChars(str, cstr);
  return std::string(cstr);
}


