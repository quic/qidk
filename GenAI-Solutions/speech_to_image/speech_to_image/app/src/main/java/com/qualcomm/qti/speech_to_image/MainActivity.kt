//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.qualcomm.qti.speech_to_image

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.ContextCompat

var cacheDirPath = ""

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        cacheDirPath = cacheDir.absolutePath
        val nativeDirPath: String = this.getApplicationInfo().nativeLibraryDir

        if(0 != PrepareNativeJNI.copyAssetsToCache(getAssets(), cacheDirPath)){
            throw Exception("Error in app preparation")
        }

        if(0 != SpeechToImageNativeJNI.initializeApp(cacheDirPath)){
            throw Exception("Error in stable diffusion app initialization")
        }

        if(0 != SpeechToImageNativeJNI.initializeTokenizer(cacheDirPath + "/tokenizer.json")){
            throw Exception("Error in tokenizer initialization")
        }

        if(0 != SpeechToImageNativeJNI.initializeDetokenizer(cacheDirPath + "/detokenizer.json")){
            throw Exception("Error in tokenizer initialization")
        }

        if(isRecordAudioPermissionGranted()){
            setContent {
                DarkTheme {
                    ChatView()
                }
            }
        }else{
            requestAudioPermission();
        }
    }


    override fun onDestroy() {
        super.onDestroy()
        SpeechToImageNativeJNI.freeApp()
        SpeechToImageNativeJNI.freeTokenizer()
        clearCacheDir(cacheDirPath)
    }


    private fun requestAudioPermission(){
        requestPermissionLauncher.launch(Manifest.permission.RECORD_AUDIO)
    }

    private fun isRecordAudioPermissionGranted(): Boolean {
        return PackageManager.PERMISSION_GRANTED == ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO);
    }

    private val requestPermissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { isGranted: Boolean ->
        if (isGranted) {
            setContent {
                ChatView()
            }
        } else {
            setContent {
                PermissionNotGrantedView()
            }
            Toast.makeText(this, "Permission denied", Toast.LENGTH_SHORT).show()
        }
    }

    @Composable
    fun PermissionNotGrantedView(){
        Text(
            text = "You must give the storage access permissions to the application.",
            fontSize = 30.sp,
            modifier = Modifier.padding(top = 15.dp)
        )
    }
}

@Preview(showBackground = true)
@Composable
fun DefaultPreview() {
    ChatView()
}

fun getCacheDir() : String {
    return cacheDirPath;
}