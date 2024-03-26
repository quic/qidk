/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

/* Changes from QuIC are provided under the following license:

Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

SPDX-License-Identifier: BSD-3-Clause
==============================================================================*/

package com.example.automatic_speech_recognition;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.WorkerThread;

import com.chaquo.python.PyObject;

import java.util.Arrays;

/** Interface to load SNPE model and provide predictions. */
public class ASRHelper {
  private static final String TAG = "ASRHelper";

  private static boolean doSnpeInit = true;

  private final Context context;
  private AssetManager assetManager;

  static {
    System.loadLibrary("ASR");
  }
  public ASRHelper(Context context) {
    this.context = context;
  }

  @WorkerThread
  public synchronized String loadModel() {
    String uiLogger = "";
    try {
      // query runtimes & init SNPE
      if (doSnpeInit) {
        String nativeDirPath = context.getApplicationInfo().nativeLibraryDir;
        Log.i(TAG, "Native Path: "+nativeDirPath);
        uiLogger += queryRuntimes(nativeDirPath);

        // init SNPE
        assetManager = context.getAssets();
        Toast.makeText(context,"Initializing SNPE",Toast.LENGTH_SHORT).show();
        Log.i(TAG, "onCreate: Initializing SNPE ...");
        uiLogger = initSNPE(assetManager);

        doSnpeInit = false;
      }
    } catch (Exception ex) {
      Log.e(TAG, ex.getMessage());
      uiLogger += ex.getMessage();
    }
    return uiLogger;
  }


  //Added a New Parameter Model
  @WorkerThread
  public synchronized float[][] predict(PyObject inputFeature) {

    String runtime="CPU";
//    String runtime="DSP";

    float[] input = inputFeature.toJava(float[].class);


    float[][] logits = new float[1][3968];

    //Log.d(TAG, String.valueOf(inputFeature));
    Log.d(TAG, "Updated array"+Arrays.toString(input));


    Log.v(TAG, "Run inference...");
    if (runtime.equals("DSP")) {
    Log.i(TAG, "Sending Inf request to SNPE DSP");
      String dsp_logs = inferSNPE(runtime,input,logits[0]);
      Log.d(TAG,"CPU Logs"+dsp_logs);

      if (! dsp_logs.isEmpty()) {
        Log.i(TAG, "DSP Exec status : " + dsp_logs);
      }
      Log.i(TAG, "predict:logits = " + Arrays.toString(logits[0]));

    } else {
        Log.d(TAG,"CPU inferece");
      //Log.i(TAG, "Sending Inf request to SNPE CPU");

      String cpu_logs = inferSNPE(runtime,input,logits[0]);
      Log.d(TAG,"CPU Logs"+cpu_logs);

      if (! cpu_logs.isEmpty()) {
        Log.i(TAG, "CPU Exec status : " + cpu_logs);
      }
      Log.i(TAG, "predict:logits = " + Arrays.toString(logits[0]));
    }

    return logits;
  }


  public native String queryRuntimes(String nativeDirPath);
  public native String initSNPE(AssetManager assetManager);
  public native String inferSNPE(String runtime, float[] input_ids, float[] logits);


}
