//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.qualcomm.qti.speech_to_image

import android.content.Context
import android.widget.Toast
import java.io.File

fun clearCacheDir(path: String) {
    val directory = File(path)
    if (directory.isDirectory) {
        directory.listFiles()?.forEach { file ->
            if (file.isFile && (file.extension == "png" || file.extension == "wav")) {
                file.delete()
            }
        }
    }
}

class ToastUtil(private val context: Context) {
    fun launch(text: String){
        Toast.makeText(
            context,
            text,
            Toast.LENGTH_SHORT
        ).show()
    }
}