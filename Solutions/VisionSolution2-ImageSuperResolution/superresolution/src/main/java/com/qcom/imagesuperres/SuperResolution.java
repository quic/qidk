//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.qcom.imagesuperres;

import android.app.Application;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Matrix;

import com.qualcomm.qti.snpe.FloatTensor;
import com.qualcomm.qti.snpe.NeuralNetwork;
import com.qualcomm.qti.snpe.SNPE;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;



public class SuperResolution {
    // Defining few model constants like input/output height widths.
//    static final Map<String, ModelConstants> modelConstants = new LinkedHashMap<String, ModelConstants>() {{
//        put("SuperResolution_sesr", new ModelConstants("Quant_SuperResolution_sesr",
//                128, 128, 256, 256, true, true));
//    }};
    static final Map<String, ModelConstants> modelConstants = new LinkedHashMap<String, ModelConstants>() {{
        put("SuperResolution_sesr", new ModelConstants("sesr_w8a8",
                128, 128, 512, 512, true, true));
    }};
    private ModelConstants model;
    private NeuralNetwork nNetwork;
    private String inputNameRGB;
    private String outputName;
    private Map<String, FloatTensor> inputs;
    private Context context;
    public boolean mUnsignedPD;

    public boolean initializingModel(Context context, String modelName, String device) {
        System.out.println("Initializing model %s."+modelName);
        this.context = context;
        Application application = (Application) context.getApplicationContext();
        if (nNetwork != null) {
            System.out.println("Already initialized.");
            return true;
        }
        model = modelConstants.get(modelName);
        if (model == null) {//sanity checks to verify if model is empty
            System.out.println("No model exists named %s."+ modelName);
            return false;
        }
        try {
            Set<String> inputLayerNames, outputLayerNames;

            String modelFile = model.getModelName() + ".dlc";
            InputStream modelData = application.getAssets().open(modelFile);//to read dlc model file from asset manager
            NeuralNetwork.RuntimeCheckOption runtimeCheck = NeuralNetwork.RuntimeCheckOption.NORMAL_CHECK;
            mUnsignedPD=true;
            if (mUnsignedPD){
                runtimeCheck = NeuralNetwork.RuntimeCheckOption.UNSIGNEDPD_CHECK;
                // runtimecheck = UNSIGNEDPD_CHECK
            }
            int modelSize = modelData.available();// To get the model size
            NeuralNetwork.Runtime runtime = NeuralNetwork.Runtime.valueOf(device);
            System.out.println("Model loaded successfully. Building network on %s."+ runtime);
            //NN building using SNPE on the desired runtime (CPU, GPU or DSP)

            final SNPE.NeuralNetworkBuilder builder = new SNPE.NeuralNetworkBuilder(application)
                    .setRuntimeCheckOption(runtimeCheck)
                    .setDebugEnabled(false)
                    .setRuntimeOrder(runtime)
                    .setModel(modelData, modelSize)
                    .setCpuFallbackEnabled(true)
                    .setUseUserSuppliedBuffers(false)
                    .setExecutionPriorityHint(NeuralNetwork.ExecutionPriorityHint.HIGH);

            nNetwork = builder.build();
            System.out.println("Network built successfully.");
            inputLayerNames = nNetwork.getInputTensorsNames();
            outputLayerNames = nNetwork.getOutputTensorsNames();
            Iterator<String> it = inputLayerNames.iterator();
            inputNameRGB = it.next();
            outputName = outputLayerNames.iterator().next();
            final FloatTensor inputTensor_RGB = nNetwork.createFloatTensor(
                    nNetwork.getInputTensorsShapes().get(inputNameRGB));
            inputs = new HashMap<>();
            inputs.put(inputNameRGB, inputTensor_RGB);

        } catch (IllegalStateException | IOException e) {
            System.out.println(e.getMessage()+ e);
            freeNetwork();
        }

        return nNetwork != null;
    }
    //imgModelInputProportion : Method to change input image size to model input layer size
    public Bitmap[] imgModelInputProportion(int sizeOperation, Bitmap[] images, Bitmap[] processReadyImages){
        switch (sizeOperation) {
            case 0:// passing input image as it is
                processReadyImages = images;
                break;
            case 1:// input image dimensions are same as model input layer dimensions
                for (int i = 0; i < images.length; i++) {
                    processReadyImages[i] = Bitmap.createBitmap(images[i], 0, 0, images[i].getWidth(), images[i].getHeight());
                }
                break;
            case 2://input image dimensions != model input dimension; But, both height and width is different in same proportion
                for (int i = 0; i < images.length; i++) {
                    processReadyImages[i] = Bitmap.createScaledBitmap(images[i], model.getInputWidth(), model.getInputHeight(), true);
                }
                break;
            default:
                throw new IllegalArgumentException("wrong size operation given");
        }
        return processReadyImages;
    }

    public Bitmap[] imgModelOutputProportion(int sizeOperation, Bitmap[] images, Bitmap[] revertedProcessedImages, Bitmap[] result){
        switch (sizeOperation) {
            case 0:
                revertedProcessedImages = images;
                break;
            case 1:
                for (int i = 0; i < result.length; i++) {
                    revertedProcessedImages[i] = Bitmap.createBitmap(result[i], 0, 0, result[i].getWidth(), result[i].getHeight());
                }
                break;
            case 2:
                for (int i = 0; i < result.length; i++) {
                    revertedProcessedImages[i] = Bitmap.createScaledBitmap(result[i], images[i].getWidth(), images[i].getHeight(), true);
                }
                break;
            default:
                throw new IllegalArgumentException("wrong size operation given");
        }
        return revertedProcessedImages;
    }

    public Result<SuperResolutionResult> process(Bitmap[] images, int sizeOperation) {
        System.out.println("Processing %d images %dx%d."+ images.length+ images[0].getWidth()+ images[0].getHeight());

        if (nNetwork == null) {
            System.out.println("Not initialized.");
            return null;
        }

        try {
            Bitmap[] processReadyImages = new Bitmap[images.length];
            processReadyImages = imgModelInputProportion(sizeOperation, images, processReadyImages);

            float[] preprocImage = PrePostProcess.preprocess(processReadyImages,model);
            inputs.get(inputNameRGB).write(preprocImage, 0, preprocImage.length);

            long inferenceStartTime = System.nanoTime();
            final Map<String, FloatTensor> outputs = nNetwork.execute(inputs);// getting inference on preprocessed input image
            long inferenceEndTime = System.nanoTime();
            System.out.println("Inference time: "+ (inferenceEndTime - inferenceStartTime) / 1000);// calculated inference time

            final float[] buff = new float[outputs.get(outputName).getSize()];
            System.out.println("shubham outputName: "+ outputName +"buff.length: "+buff.length);

//            float[] array = new float[10];
            for (Map.Entry<String, FloatTensor> output : outputs.entrySet()) {
                if (output.getKey().equals(outputName)) {
                    FloatTensor outputTensor = output.getValue();
                    outputTensor.read(buff, 0, buff.length);}
            }

//            outputs.get(outputName).read(buff, 0, buff.length);// reading output from output layer
            Bitmap[] result = PrePostProcess.postprocess(buff,model);// post processing output to a bitmap

//            Bitmap[] result = PrePostProcess.postprocess(array,model);// post processing output to a bitmap
            Bitmap[] finalProcessedImages = new Bitmap[images.length];
            finalProcessedImages = imgModelOutputProportion(sizeOperation, images, finalProcessedImages, result);

            List<SuperResolutionResult> results = new ArrayList<>();
            results.add(new SuperResolutionResult(finalProcessedImages));
            return new Result<>(results,
                    (inferenceEndTime - inferenceStartTime) / 1000000);

        } catch (Exception ex) {
            ex.printStackTrace();
        }
        return null;
    }

    public void freeNetwork() {
        if (nNetwork != null) {
            nNetwork.release();
            model = null;
            nNetwork = null;
        }
    }
}
