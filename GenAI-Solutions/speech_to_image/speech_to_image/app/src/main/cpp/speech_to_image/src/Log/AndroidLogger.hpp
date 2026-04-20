//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#ifndef TEXT_TO_IMAGE_ANDROIDLOGGER_HPP
#define TEXT_TO_IMAGE_ANDROIDLOGGER_HPP

#include <android/log.h>

#define TAG "SpeechToImage"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// Function to redirect cout and cerr
void redirect_cout_cerr_to_logcat();

#endif //TEXT_TO_IMAGE_ANDROIDLOGGER_HPP
