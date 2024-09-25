//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.example.automatic_speech_recognition;


import static android.widget.Toast.LENGTH_SHORT;
import static android.widget.Toast.makeText;

import android.annotation.SuppressLint;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;


import org.tensorflow.lite.Interpreter;;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.LinkedBlockingQueue;


public class MainActivity extends AppCompatActivity {
    private Button startRecordButton, stopRecordButton,playButton;
    private TextView audioText;
    private AudioRecord audioRecord;
    private static final String TAG = "ASRHelper";
    private boolean isRecording = false;
    private int sampleRate = 16000;
    private int bufferSize;
    private BlockingQueue<byte[]> audioQueue = new LinkedBlockingQueue<>();
    final String[] prevMessage = new String[]{""};
    int mutex=0;


    private native String InitSnpe(AssetManager assets, String nativeLibraryDir);
    private native String LoadVocab(AssetManager assets);
    private native String Transcribe(AssetManager assets, long[] encoder_hidden_state,int len);
//    public native String queryRuntimes(String a);

    static {
        System.loadLibrary("ASR");
    }


    @SuppressLint("MissingInflatedId")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //Initializing Buttons and TextView
        startRecordButton = findViewById(R.id.startRecordButton);
        stopRecordButton = findViewById(R.id.stopRecordButton);
        playButton=findViewById(R.id.playButton);
        audioText= findViewById(R.id.text);
        playButton.setVisibility(View.GONE);


        long start_moduleTime=System.currentTimeMillis();
        String initLogs =loadModel();
        long end_moduleTime=System.currentTimeMillis();
        long elapsedTime_module=end_moduleTime-start_moduleTime;
        Toast.makeText(MainActivity.this,"Encoder+Decoder Model Load Time: "+elapsedTime_module+"ms",Toast.LENGTH_LONG).show();


        Log.i(TAG, "initLogs: "+initLogs);
       //When startRecordButton is clicked
        startRecordButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!isRecording) {
                    startRecording();
                    startRecordButton.setEnabled(false);
                    stopRecordButton.setEnabled(true);
                }
            }
        });



        stopRecordButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (isRecording) {
                    stopRecording();
                    startRecordButton.setEnabled(true);
                    stopRecordButton.setEnabled(false);
                }
            }
        });


    }
    private static boolean doSnpeInit = true;
    private TFLiteInference tfliteInference;
    public String loadModel() {
        String uiLogger = "";
        try {
            //  initializing SNPE


//            String res_query = queryRuntimes(getApplicationInfo().nativeLibraryDir);
            if (doSnpeInit) {
                InitSnpe(getAssets(),getApplicationInfo().nativeLibraryDir);
                LoadVocab(getAssets());


            }
        } catch (Exception ex) {
            Log.e(TAG, ex.getMessage());
        }
        //Initializing the Decoder Model

        try {
            tfliteInference= new TFLiteInference(MainActivity.this, "whisper-decoder-tiny.tflite");
            if (tfliteInference.isModelLoaded()) {
                int inputTensorCount = tfliteInference.getInputTensorCount();
                Log.d(TAG, "Number of Input Tensors: " + inputTensorCount);
                // Get information about each input tensor
                for (int i = 0; i < inputTensorCount; i++) {
                    int[] inputTensorShape = tfliteInference.getInputTensorShape(i);
                    Log.d(TAG, "Input Tensor Shape for Input " + i + ": " + Arrays.toString(inputTensorShape));
                }
                // Continue with your inference or other operations
            } else {
                Log.e(TAG, "Model failed to load.");
            }
        } catch (IOException e) {
            e.printStackTrace();
            Log.e(TAG, "Error loading model: " + e.getMessage());
        }
        return uiLogger;
    }




    //Latch to start the Recording Thread First, Then the Transcription Thread
    private CountDownLatch recordingLatch = new CountDownLatch(1);
    private void startRecording() {
        Log.d(TAG,"Recording started");
        makeText(this, "Recording Started", LENGTH_SHORT).show();
        bufferSize = AudioRecord.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);

        //Give the MIC permission from the Android app
        if (ActivityCompat.checkSelfPermission(this, android.Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            Toast.makeText(this,"Give Permission From Setting",Toast.LENGTH_LONG).show();
            return;
        }
        audioText.setText("Recording Started");
        prevMessage[0] ="";
        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, sampleRate, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT, bufferSize);

        if (audioRecord.getState() == AudioRecord.STATE_INITIALIZED) {
            audioRecord.startRecording();
            isRecording = true;

            //Creating 2 Thread
            //One Thread is for Recording
            //Another is for Transcribtion task
            Thread recordingThread = new Thread(new AudioRecordingTask());
            Thread transcriptionThread = new Thread(new TranscriptionTask());

            recordingThread.start();

            try {
                recordingLatch.await();  // Wait for recordingThread to complete the first chunk
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            transcriptionThread.start();
        }
    }

    private class AudioRecordingTask implements Runnable {
        @Override
        public void run() {

            //Here We've used fixed chunk Size
            //TODO Need to use variable chunk size to detect pause and based on that divide the array
            int chunkSize = 50000;

            byte[] audioChunk = new byte[chunkSize];

            // Signal that recording is ready
            recordingLatch.countDown();

            while (isRecording) {
                int bytesRead = audioRecord.read(audioChunk, 0, chunkSize);

                if (bytesRead > 0) {
                    try {
                        audioQueue.put(audioChunk);
                    } catch (InterruptedException e) {
                        throw new RuntimeException(e);
                    }

                    // Log the updated size of audioQueue
                    int queueSize = audioQueue.size();
                    Log.d(TAG, "Updated audioQueue size: " + queueSize);

                    // Prepare the next audio chunk for recording
                    audioChunk = new byte[chunkSize];

                    // Log a message indicating that the next audio chunk is ready for recording
                    Log.d(TAG, "Next audio chunk is ready for recording");
                }
            }
        }
    }




    private class TranscriptionTask implements Runnable {
        private static final int TIMER_INTERVAL = 120;
        private int mPeriodInFrames = sampleRate * TIMER_INTERVAL / 1000;



        private short nChannels=1;
        private int sRate=16000;
        private short mBitsPersample=16;
        private int count=0;



        @Override
        public void run() {
            //Log.d(TAG,"Mutex Valu before while_1: "+mutex);
            //ReentrantLock mutex = new ReentrantLock();
            while (isRecording) {
               // mutex.lock();
                CountDownLatch transcriptionLatch = new CountDownLatch(2);

                //Log.d(TAG,"Mutex Value after while_2: "+mutex);
                byte[] audioChunk = audioQueue.poll();

                if (audioChunk != null) {
                    Log.d(TAG,"Taking :"+count+" Audio data");
                    String filePath =  getExternalCacheDir().getAbsolutePath();
                    filePath += "/android_record_"+count +".wav";
                    Log.e(TAG, filePath);
                    RandomAccessFile randomAccessWriter = null;
                    try {
                        randomAccessWriter = new RandomAccessFile(filePath, "rw");
                    } catch (FileNotFoundException e) {
                        throw new RuntimeException(e);
                    }
                    for(int i=0;i<4;i++){
                        Log.d(TAG,"Audio "+audioChunk[i]);
                    }
                    try {
                        prepare(randomAccessWriter);
                        //Checking the proper buffer size
                        Log.d(TAG,"After Trascribtion Done for data"+count+" buffer length: "+ sRate * nChannels * mBitsPersample / 8+"Audio Length:"+audioChunk.length);

                        try {

                        randomAccessWriter.write(audioChunk);
                        randomAccessWriter.seek(4); // Write size to RIFF header
                        randomAccessWriter.writeInt(Integer.reverseBytes(36 + audioChunk.length));
                        randomAccessWriter.seek(40); // Write size to Subchunk2Size field
                        randomAccessWriter.writeInt(Integer.reverseBytes(audioChunk.length));

                            randomAccessWriter.close();
                        } catch (IOException e) {
                            Log.e(TAG,"Error in closing");
                            throw new RuntimeException(e);
                        }


                    } catch (Exception e) {
                        e.printStackTrace();
                        Log.d(TAG,"Error in Transcribing"+String.valueOf(e));
                    }



                    Log.d(TAG,"After Trascribtion Done for data"+count+" latch count: "+transcriptionLatch.getCount());

                    Log.d(TAG,"Before Trascribtion for data"+count);
                    transcribeAudioChunk(filePath,count,transcriptionLatch);

                    try {
                        Log.d(TAG,"Waiting in Latch for data"+count+" latch count: "+transcriptionLatch.getCount());
                        transcriptionLatch.await();  // Wait for recordingThread to complete the first chunk
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }

                    Log.d(TAG,"Everything is completed for data:"+count++);
                }

            }


        }

        public void prepare(RandomAccessFile randomAccessWriter) {
            try {
                randomAccessWriter.setLength(0); // Set file length to 0, to prevent unexpected behavior in case the file already existed
                randomAccessWriter.writeBytes("RIFF");
                randomAccessWriter.writeInt(0); // Final file size not known yet, write 0
                randomAccessWriter.writeBytes("WAVE");
                randomAccessWriter.writeBytes("fmt ");
                randomAccessWriter.writeInt(Integer.reverseBytes(16)); // Sub-chunk size, 16 for PCM
                randomAccessWriter.writeShort(Short.reverseBytes((short) 1)); // AudioFormat, 1 for PCM
                randomAccessWriter.writeShort(Short.reverseBytes(nChannels));// Number of channels, 1 for mono, 2 for stereo
                randomAccessWriter.writeInt(Integer.reverseBytes(sRate)); // Sample rate
                randomAccessWriter.writeInt(Integer.reverseBytes(sRate * nChannels * mBitsPersample / 8)); // Byte rate, SampleRate*NumberOfChannels*mBitsPersample/8
                randomAccessWriter.writeShort(Short.reverseBytes((short) (nChannels * mBitsPersample / 8))); // Block align, NumberOfChannels*mBitsPersample/8
                randomAccessWriter.writeShort(Short.reverseBytes(mBitsPersample)); // Bits per sample
                randomAccessWriter.writeBytes("data");
                randomAccessWriter.writeInt(0); // Data chunk size not known yet, write 0

                } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }


    }


    private void transcribeAudioChunk(String audioChunk, int count, CountDownLatch transcriptionLatch) {


        new Thread(new Runnable() {
            @Override
            public void run() {
                if (audioChunk != null) {

                    Log.d(TAG, "Chunk Path: " + audioChunk+": data "+count);
                    float[][] input_features = new float[1][240000];
                    float[][] encoder_hidden_state = new float[1][576000];
                    long[][] decoder_ids={{50258, 50259, 50359, 50363}};

                    String runtime="DSP";

                    Log.d(TAG, "Before Encoder Inference for: data "+count+" latch:"+transcriptionLatch.getCount());

                    //Running The Encoder Model on DSP
                    loadModelJNI(getAssets(),1,audioChunk,input_features[0],encoder_hidden_state[0],runtime);

                    transcriptionLatch.countDown();
                    Log.d(TAG, "After Encoder Inference for: data "+count+" latch:"+transcriptionLatch.getCount());

                    Log.i(TAG, "predict:logits = " + Arrays.toString(encoder_hidden_state[0]));
                    float[][][] hidden_state = new float[1][1500][384];
                    int index = 0;
                    for (int i = 0; i < 1; i++) {
                        for (int j = 0; j < 1500; j++) {
                            for (int k = 0; k < 384; k++) {
                                hidden_state[i][j][k] = encoder_hidden_state[0][index++];
                            }
                        }
                    }

                    //Inferencing the TFLite Model

                    try{
                        if(tfliteInference!=null && tfliteInference.isModelLoaded()){
                            infer(hidden_state,decoder_ids,transcriptionLatch);
                        }

                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }

                }
            }
        }).start();
    }
    public void infer(float[][][] input1Data, long[][] input2Data, CountDownLatch transcriptionLatch) {


        new Thread(new Runnable() {
            @Override
            public void run() {

                Interpreter tflite = tfliteInference.getTflite();
                if (tflite == null) {
                    throw new IllegalStateException("Model not loaded properly.");
                }


                long[] temp = input2Data[0];
                long[][] inp2data;
                // Run inference
                Object[] inputs = {input1Data, input2Data};
                int decoder_legth = input2Data[0].length;



                while (true) {

                    Map<Integer, Object> outputs = new HashMap<>();
                    float[][][] output = new float[1][decoder_legth][51865];

                    outputs.put(0, output);
                    try {
                        tflite.runForMultipleInputsOutputs(inputs, outputs);
                    } catch (Exception e) {
                        e.printStackTrace();
                        throw new RuntimeException(e);
                    }
                    int[] tokens = new int[decoder_legth];
                    for (int i = 0; i < decoder_legth; i++) {
                        tokens[i] = findIndexOfMax(output[0][i]);
                    }
                    Log.i(TAG, "predict:tokens = " + Arrays.toString(tokens));

                    //Initializing new input
                    decoder_legth++;
                    //Log.i(TAG, "updated++ Decoder Length = " + decoder_legth);
                    inp2data = new long[1][decoder_legth];
                    //copying the decoder input_ids
                    for (int i = 0; i < decoder_legth - 1; i++) {
                        inp2data[0][i] = temp[i];
                    }

                    inp2data[0][decoder_legth - 1] = tokens[tokens.length - 1];
                    temp = inp2data[0];
                    //Log.i(TAG, "predict:updated input = " + Arrays.toString(inp2data[0]));
                    inputs = new Object[]{input1Data, inp2data};
                    Log.i(TAG, "Normal Decoder_Length" + decoder_legth);

                    if (tokens[tokens.length - 1] == 50257) {
                        break;
                    }


                }

                String res = Transcribe(getAssets(), inp2data[0], inp2data[0].length).substring(67);
                transcriptionLatch.countDown();
                Log.i(TAG, "logits after everything= " + res);
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        // Update the TextView with the result on the UI thread
                        prevMessage[0]+=res;
                        audioText.setText(prevMessage[0]);

                    }
                });
            }


        }).start();

        }



    public static int findIndexOfMax(float[] array) {
        float max = Float.NEGATIVE_INFINITY;
        int maxIndex = -1;
        for (int i = 0; i < array.length; i++) {
            if (array[i] > max) {
                max = array[i];
                maxIndex = i;
            }
        }
        return maxIndex;
    }

    private void stopRecording() {
        isRecording = false;
        if (audioRecord != null) {
            audioRecord.stop();
            audioRecord.release();
            audioRecord = null; // Set to null after release to avoid double release
        }
    }
    private native String loadModelJNI(AssetManager assetManager, int is_recorded, String audio_data, float[] inputFeature, float[] encoder_hidden_state, String runtime);


}
