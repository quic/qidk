//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.qualcomm.qti.speech_to_image

import android.content.res.AssetManager

object PrepareNativeJNI {
    init {
        System.loadLibrary("prepare-native")
    }

    external fun copyAssetsToCache(jassetManager: AssetManager, cacheDir: String) : Int;
}