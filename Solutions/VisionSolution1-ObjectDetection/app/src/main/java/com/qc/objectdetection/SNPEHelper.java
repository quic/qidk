//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.qc.objectdetection;

import android.app.Application;
import android.graphics.Bitmap;
import android.util.ArrayMap;

import com.qualcomm.qti.snpe.FloatTensor;
import com.qualcomm.qti.snpe.NeuralNetwork;
import com.qualcomm.qti.snpe.SNPE;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;


public class SNPEHelper {
    private final Application mApplication;
    private final BitmapUtility mBitmapUtility;
    private FloatTensor tempInputTensor;
    private NeuralNetwork mNeuralNetwork;
    private Map<String, FloatTensor> mInputTensorsMap;
    private int[] mInputTensorHWC;
    // Constructor
    public SNPEHelper(Application application) {
        mApplication = application;
        mBitmapUtility = new BitmapUtility();
    }
    // Method to get height of input layer NeuralNet
    public int getInputHeight() {
        if (mInputTensorHWC == null)
            return 0;
        else
            return mInputTensorHWC[2];
    }
    // Method to get width of input layer NeuralNet
    public int getInputWidth() {
        if (mInputTensorHWC == null)
            return 0;
        else
            return mInputTensorHWC[1];
    }

    private static final String MNETSSD_MODEL_ASSET_NAME = "object_detect.dlc";
    private static final String MNETSSD_INPUT_LAYER = "data";
    private static final String MNETSSD_OUTPUT_LAYER ="detection_out";
    private static final String MNETSSD_OUTPUT_LAYER1 = "detection_out_scores";
    private static final String MNETSSD_OUTPUT_LAYER2 = "detection_out_boxes";
    private static final String MNETSSD_OUTPUT_LAYER3 = "detection_out_classes";
    private static final String MNETSSD_OUTPUT_LAYER4 = "detection_out_num_detections";
    private static final boolean MNETSSD_NEEDS_CPU_FALLBACK = true;
    private static int MNETSSD_NUM_BOXES = 100;
    private final float[] floatOutput1 = new float[MNETSSD_NUM_BOXES];
    private final float[] floatOutput2 = new float[MNETSSD_NUM_BOXES * 4];
    private final float[] floatOutput3 = new float[MNETSSD_NUM_BOXES];
    private final float[] floatOutput4 = new float[1];
    private final ArrayList<RectangleBox> mSSDBoxes = RectangleBox.createBoxes(MNETSSD_NUM_BOXES);

    /**
     * This method loads MobileNetSSD model on selected runtime
     */

    public boolean loadingMNETSSD(char runtime_var) {
        disposeNeuralNetwork();//remove any previous instance
        System.out.println("runtime_var :: "+runtime_var);

        if (runtime_var=='G') {
            System.out.println("Running on GPU");
            NeuralNetwork.Runtime selectedCore = NeuralNetwork.Runtime.GPU;
            mNeuralNetwork = loadingMNETSSDFromDLC(mApplication, MNETSSD_MODEL_ASSET_NAME,
                    selectedCore, MNETSSD_NEEDS_CPU_FALLBACK, MNETSSD_OUTPUT_LAYER);//MNETSSD_OUTPUT_LAYER1, MNETSSD_OUTPUT_LAYER2, MNETSSD_OUTPUT_LAYER3, MNETSSD_OUTPUT_LAYER4);
        }
        else if (runtime_var=='D'){
            System.out.println("Running on DSP");
            NeuralNetwork.Runtime selectedCore = NeuralNetwork.Runtime.DSP;
            mNeuralNetwork = loadingMNETSSDFromDLC(mApplication, MNETSSD_MODEL_ASSET_NAME,
                    selectedCore, MNETSSD_NEEDS_CPU_FALLBACK, MNETSSD_OUTPUT_LAYER);//MNETSSD_OUTPUT_LAYER1, MNETSSD_OUTPUT_LAYER2, MNETSSD_OUTPUT_LAYER3, MNETSSD_OUTPUT_LAYER4);
        }
        if (mNeuralNetwork == null) {
            if (runtime_var == 'G'){
                System.out.println("GPU_FLOAT16 runtime FAILED");
            }
            else if (runtime_var == 'D'){
                System.out.println("DSP runtime FAILED");
            }
            System.out.println("Running on CPU");
            mNeuralNetwork = loadingMNETSSDFromDLC(mApplication, MNETSSD_MODEL_ASSET_NAME,
                    NeuralNetwork.Runtime.CPU, MNETSSD_NEEDS_CPU_FALLBACK,  MNETSSD_OUTPUT_LAYER);//MNETSSD_OUTPUT_LAYER1, MNETSSD_OUTPUT_LAYER2, MNETSSD_OUTPUT_LAYER3, MNETSSD_OUTPUT_LAYER4);
        }

        if (mNeuralNetwork != null)
            mInputTensorHWC = mNeuralNetwork.getInputTensorsShapes().get(MNETSSD_INPUT_LAYER);
        else
            mInputTensorHWC = new int[0];

        tempInputTensor = mNeuralNetwork.createFloatTensor(mInputTensorHWC);
        mInputTensorsMap = new HashMap<>();
        mInputTensorsMap.put(MNETSSD_INPUT_LAYER, tempInputTensor);
        return true;
    }
    /**
     * This method initiates inference on bitmap and post-process the inferred results
     */
    public ArrayList<RectangleBox> snpeInference(Bitmap modelInputBitmap, long tic, int fps) {
        try {
            final Map<String, FloatTensor> outputs = bitmapInference(modelInputBitmap);
            if (outputs == null)
                return null;
            //Count of boxes inferred
            outputs.get(MNETSSD_OUTPUT_LAYER4).read(floatOutput4, 0, 1);
            MNETSSD_NUM_BOXES = (int) floatOutput4[0];
            //inferred coordinates
            outputs.get(MNETSSD_OUTPUT_LAYER2).read(floatOutput2, 0, MNETSSD_NUM_BOXES * 4);
            // inferred box labels
            outputs.get(MNETSSD_OUTPUT_LAYER3).read(floatOutput3, 0, MNETSSD_NUM_BOXES);
            // inferred box confidence
            outputs.get(MNETSSD_OUTPUT_LAYER1).read(floatOutput1, 0, MNETSSD_NUM_BOXES);

            for (int i = 0; i < MNETSSD_NUM_BOXES; i++) {
                float mSSDOutputBoxes[] = new float[MNETSSD_NUM_BOXES * 4];
                float mSSDOutputClasses[] = new float[MNETSSD_NUM_BOXES];
                float mSSDOutputScores[] = new float[MNETSSD_NUM_BOXES];
                mSSDOutputBoxes[(i * 4)] = floatOutput2[0 + (4 * i)];
                mSSDOutputBoxes[1 + (i * 4)] = floatOutput2[1 + (4 * i)];
                mSSDOutputBoxes[2 + (i * 4)] = floatOutput2[2 + (4 * i)];
                mSSDOutputBoxes[3 + (i * 4)] = floatOutput2[3 + (4 * i)];
                mSSDOutputClasses[i] = floatOutput3[i];
                mSSDOutputScores[i] = floatOutput1[i];
                RectangleBox rbox = mSSDBoxes.get(i);
                rbox.top = mSSDOutputBoxes[(i * 4)];
                rbox.left = mSSDOutputBoxes[1 + (4 * i)];
                rbox.bottom = mSSDOutputBoxes[2 + (4 * i)];
                rbox.right = mSSDOutputBoxes[3 + (4 * i)];
                rbox.label_index = (int) mSSDOutputClasses[i];
                rbox.confidence = mSSDOutputScores[i];
                rbox.label = lookupClass(rbox.label_index, "cannot get label name");
                long toc = System.currentTimeMillis();
                System.out.println("tic : "+tic+" toc : "+toc);
                long totalTime = (toc - tic);
                System.out.println("TotalTime :: "+totalTime);
                int processing_time = (int) (totalTime);
                rbox.processing_time = processing_time;
                rbox.fps = fps;
                System.out.println("processing_time "+String.valueOf(processing_time)+" || "+processing_time);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return mSSDBoxes;
    }
    /**
     * This method makes inference on bitmap
     */

    private Map<String, FloatTensor> bitmapInference(Bitmap inputBitmap) {
        final Map<String, FloatTensor> outputs;
        try {
            // safety check
            if (mNeuralNetwork == null || tempInputTensor == null || inputBitmap.getWidth() != getInputWidth() || inputBitmap.getHeight() != getInputHeight()) {
                return null;
            }

            mBitmapUtility.convertBitmapToBuffer(inputBitmap);
            final float[] inputFloatsHW3 = mBitmapUtility.bufferToFloatsBGR();
            if (mBitmapUtility.isBufferBlack())
                return null;
            tempInputTensor.write(inputFloatsHW3, 0, inputFloatsHW3.length, 0, 0);
            outputs = mNeuralNetwork.execute(mInputTensorsMap);
        } catch (Exception e) {
            e.printStackTrace();
            System.out.println(e.getCause() + "");
            return null;
        }
        return outputs;
    }
    /**
     * This method initializes SNPE with MobileNetSSD DLC
     */
    private static NeuralNetwork loadingMNETSSDFromDLC(
            Application application, String assetFileName, NeuralNetwork.Runtime selectedRuntime,
            boolean needsCpuFallback, String... outputLayerNames) {
        try {
            // input stream to read from the assets
            InputStream assetInputStream = application.getAssets().open(assetFileName);
            NeuralNetwork.RuntimeCheckOption runtimeCheck = NeuralNetwork.RuntimeCheckOption.NORMAL_CHECK;
            boolean mUnsignedPD=true;
            if (mUnsignedPD){
                runtimeCheck = NeuralNetwork.RuntimeCheckOption.UNSIGNEDPD_CHECK;
                // runtimecheck = UNSIGNEDPD_CHECK
            }
            // create the neural network
            System.out.println("create the neural network");
            NeuralNetwork network = new SNPE.NeuralNetworkBuilder(application)
                    .setRuntimeCheckOption(runtimeCheck)
                    .setDebugEnabled(false)
                    .setOutputLayers(outputLayerNames)
                    .setModel(assetInputStream, assetInputStream.available())
                    .setPerformanceProfile(NeuralNetwork.PerformanceProfile.DEFAULT)
                    .setRuntimeOrder(selectedRuntime) // Runtime.DSP, Runtime.GPU_FLOAT16, Runtime.GPU, Runtime.CPU
                    .setCpuFallbackEnabled(needsCpuFallback)
                    .build();
            System.out.println("neural network created");
            // close input
            assetInputStream.close();
            System.out.println("Network runtime :: "+selectedRuntime);
            // all right, network loaded
            return network;
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        } catch (IllegalStateException | IllegalArgumentException e2) {
            e2.printStackTrace();
            return null;
        }
    }

    public void disposeNeuralNetwork() {
        if (mNeuralNetwork == null)
            return;
        else {
            mNeuralNetwork.release();
            mNeuralNetwork = null;
            mInputTensorHWC = null;
            tempInputTensor = null;
            mInputTensorsMap = null;
        }
    }



    private Map<Integer, String> mCocoMap;

    /**
     * This method has all class labels and index
     */
    private String lookupClass(int cocoIndex, String fallback) {

        if (mCocoMap == null) {
            mCocoMap = new ArrayMap<>();
            mCocoMap.put(0, "background");
            mCocoMap.put(1, "aeroplane");
            mCocoMap.put(2, "bicycle");
            mCocoMap.put(3, "bird");
            mCocoMap.put(4, "boat");
            mCocoMap.put(5, "bottle");
            mCocoMap.put(6, "bus");
            mCocoMap.put(7, "car");
            mCocoMap.put(8, "cat");
            mCocoMap.put(9, "chair");
            mCocoMap.put(10, "cow");
            mCocoMap.put(11, "diningtable");
            mCocoMap.put(12, "dog");
            mCocoMap.put(13, "horse");
            mCocoMap.put(14, "motorbike");
            mCocoMap.put(15, "person");
            mCocoMap.put(16, "pottedplant");
            mCocoMap.put(17, "sheep");
            mCocoMap.put(18, "sofa");
            mCocoMap.put(19, "train");
            mCocoMap.put(20, "tvmonitor");

        }
        if (mCocoMap.containsKey(cocoIndex)){
            return mCocoMap.get(cocoIndex);
        }
        else{
            return fallback;
        }

    }


}