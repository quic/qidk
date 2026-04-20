//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>
#include <fstream>
#include <filesystem>
#include <android/log.h>
#include <vector>

#define TAG "speech_to_image"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

bool copy_asset_to_cache_util(AAssetManager*, std::string&, const std::string&);
bool copy_assets_to_cache(AAssetManager* ,std::string&, const std::vector<std::string>&);
std::string jstring_to_stdstring(JNIEnv*, jstring);

extern "C"
{
  JNIEXPORT jint JNICALL
  Java_com_qualcomm_qti_speech_1to_1image_PrepareNativeJNI_copyAssetsToCache(JNIEnv *env,
                                                                             jobject thiz,
                                                                             jobject jasset_manager,
                                                                             jstring cache_dir) {
    std::string cacheDirStr = jstring_to_stdstring(env, cache_dir) + "/";

    AAssetManager *assetManager = AAssetManager_fromJava(env, jasset_manager);

    if(!copy_assets_to_cache(assetManager, cacheDirStr, {"text_encoder.bin", "unet.bin", "vae_decoder.bin", "tokens.raw", "tokenizer.json", "detokenizer.json",
                                                                       "t_emb_0.raw", "t_emb_1.raw", "t_emb_2.raw", "t_emb_3.raw", "t_emb_4.raw",
                                                                       "t_emb_5.raw", "t_emb_6.raw", "t_emb_7.raw", "t_emb_8.raw", "t_emb_9.raw",
                                                                       "t_emb_10.raw", "t_emb_11.raw", "t_emb_12.raw", "t_emb_13.raw", "t_emb_14.raw",
                                                                       "t_emb_15.raw", "t_emb_16.raw", "t_emb_17.raw", "t_emb_18.raw", "t_emb_19.raw",
                                                                       "whisper_encoder_tiny.bin", "whisper_decoder_tiny.bin", "mel_80.bin"})){
      LOGE("Assets copy failed, returning...\n");
      return -1;
    }

    return 0;
  }
}

bool copy_asset_to_cache_util(AAssetManager *assetManager,std::string &cache_path, const std::string &asset_name) {
  AAsset *asset = AAssetManager_open(assetManager, asset_name.c_str(), AASSET_MODE_BUFFER);

  if (asset == nullptr) {
    LOGI("Failed to open asset\n");
    return false;
  }

  const void *data = AAsset_getBuffer(asset);
  size_t size = AAsset_getLength(asset);


  FILE *file = fopen((cache_path + "/" + asset_name).c_str(), "wb");

  if (file == nullptr) {
    LOGI("Failed to open file\n");
    return false;
  }

  fwrite(data, 1, size, file);
  fclose(file);
  AAsset_close(asset);
  return true;
}

bool copy_assets_to_cache(AAssetManager *assetManager,std::string &cache_path, const std::vector<std::string> &asset_names){
  for(const std::string &asset_name: asset_names){
    if(!copy_asset_to_cache_util(assetManager, cache_path, asset_name)){
      LOGE("Error in copy to cache of asset: %s\n", asset_name.c_str());
      return false;
    }
  }
  return true;
}


std::string jstring_to_stdstring(JNIEnv* env, jstring str){
  const char *cstr = env->GetStringUTFChars(str, NULL);
  env->ReleaseStringUTFChars(str, cstr);
  return std::string(cstr);
}


