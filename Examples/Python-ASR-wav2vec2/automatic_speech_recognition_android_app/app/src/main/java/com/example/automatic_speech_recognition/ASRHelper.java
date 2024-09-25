//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

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
