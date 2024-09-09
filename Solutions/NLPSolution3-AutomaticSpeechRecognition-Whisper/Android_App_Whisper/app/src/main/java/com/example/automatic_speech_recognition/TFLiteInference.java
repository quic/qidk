package com.example.automatic_speech_recognition;


import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.util.Log;


import org.tensorflow.lite.Interpreter;

import java.io.FileInputStream;
import java.io.IOException;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;


public class TFLiteInference {
    private Interpreter tflite;

    private final static String TAG = "Sahin";



    public TFLiteInference(Context context, String modelPath) throws IOException {
        MappedByteBuffer modelBuffer = loadModelFile(context, modelPath);
        // Initialize interpreter with GPU delegate
        Interpreter.Options options = new Interpreter.Options();

        options.setNumThreads(6);



        tflite = new Interpreter(modelBuffer,options);

        Log.d(TAG,"After Initializing the TFLite Model");

        tflite.resizeInput(0, new int[]{1, 1500, 384});
        tflite.resizeInput(1, new int[]{1,4});
        tflite.getInputTensor(0);
        tflite.getInputTensor(1);


    }



    private MappedByteBuffer loadModelFile(Context context, String modelPath) throws IOException {
        AssetFileDescriptor fileDescriptor = context.getAssets().openFd(modelPath);
        FileInputStream inputStream = new FileInputStream(fileDescriptor.getFileDescriptor());
        FileChannel fileChannel = inputStream.getChannel();
        long startOffset = fileDescriptor.getStartOffset();
        long declaredLength = fileDescriptor.getDeclaredLength();
        return fileChannel.map(FileChannel.MapMode.READ_ONLY, startOffset, declaredLength);
    }
    public Interpreter getTflite(){
        return  tflite;
    }

    public boolean isModelLoaded() {
        return tflite != null;
    }
    // Get the number of input tensors
    public int getInputTensorCount() {
        return tflite.getInputTensorCount();
    }
    // Get the shape of a specific input tensor
    public int[] getInputTensorShape(int inputTensorIndex) {
        return tflite.getInputTensor(inputTensorIndex).shape();
    }
}