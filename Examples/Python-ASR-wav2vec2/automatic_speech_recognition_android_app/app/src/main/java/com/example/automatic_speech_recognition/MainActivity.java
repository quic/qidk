//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.example.automatic_speech_recognition;


import static android.widget.Toast.LENGTH_SHORT;
import static android.widget.Toast.makeText;

import android.annotation.SuppressLint;
import android.content.pm.PackageManager;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import com.chaquo.python.PyObject;
import com.chaquo.python.Python;
import com.chaquo.python.android.AndroidPlatform;

import java.util.Arrays;
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
    private BlockingQueue<short[]> audioQueue = new LinkedBlockingQueue<>();
    PyObject codeModule;
    private ASRHelper helper;

    Python python;
    final String[] prevMessage = new String[]{""};




    @SuppressLint("MissingInflatedId")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        startRecordButton = findViewById(R.id.startRecordButton);
        stopRecordButton = findViewById(R.id.stopRecordButton);
        playButton=findViewById(R.id.playButton);
        audioText= findViewById(R.id.text);
        playButton.setVisibility(View.GONE);

        //Starting the Python Interpreter
        //TODO I need to do it in separate Thread
        //Otherwise It's taking too much time
        if (!Python.isStarted()) {
            Python.start(new AndroidPlatform(MainActivity.this));
        }
        python = Python.getInstance();
        codeModule = python.getModule("model");

        //Loading the Model
        Handler model_handler = new Handler();
        helper=new ASRHelper(this);
        model_handler.post(
                () -> {
                    String initLogs = helper.loadModel();
                    Log.i(TAG, "initLogs: "+initLogs);
                    makeText(this,initLogs,Toast.LENGTH_SHORT).show();
                });

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


    private CountDownLatch recordingLatch = new CountDownLatch(1);
    private void startRecording() {
        makeText(this, "Recording Started", LENGTH_SHORT).show();
        bufferSize = AudioRecord.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);

        if (ActivityCompat.checkSelfPermission(this, android.Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            return;
        }
        audioText.setText("Nothing to show");
        prevMessage[0] ="";
        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, sampleRate, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT, bufferSize);

        if (audioRecord.getState() == AudioRecord.STATE_INITIALIZED) {
            audioRecord.startRecording();
            isRecording = true;

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
            int chunkSize = 40000;
            short[] audioChunk = new short[chunkSize];

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
                    audioChunk = new short[chunkSize];

                    // Log a message indicating that the next audio chunk is ready for recording
                    Log.d(TAG, "Next audio chunk is ready for recording");
                }
            }
        }
    }

    private class TranscriptionTask implements Runnable {
        @Override
        public void run() {
            while (isRecording) {
                short[] audioChunk = audioQueue.poll();
                if (audioChunk != null) {
                    try {
                        transcribeAudioChunk(audioChunk);
                    } catch (Exception e) {
                        e.printStackTrace();

                    }
                }
            }
        }
    }
    private void transcribeAudioChunk(short[] audioChunk) {


        new Thread(new Runnable() {
            @Override
            public void run() {
                if (audioChunk != null) {
                    int chunkSize = audioChunk.length;

                    // Show the size in a Toast message
                    final int finalChunkSize = chunkSize; // Required for using in Toast
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            makeText(MainActivity.this, "Chunk Size: " + finalChunkSize, LENGTH_SHORT).show();
                        }
                    });
                    Log.d(TAG, "Chunk Value: " + Arrays.toString(audioChunk));
                    codeModule.callAttr("main",audioChunk);
                    // Transcribe the audio chunk using your Python model
                    PyObject input_features= PyObject.fromJava(new float[40000]);
                    input_features= codeModule.callAttr("main",audioChunk);
                    Log.d(TAG, String.valueOf(input_features));

                    float[][] logits=helper.predict(input_features);

                    String transcription=codeModule.callAttr("postProcess",logits[0]).toString();
                    // Update the TextView with the result
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            // Update the TextView with the result on the UI thread
                            audioText.setText(prevMessage[0] + " " + transcription);
                            prevMessage[0] = prevMessage[0] + " " + transcription;
                        }
                    });

                }
            }
        }).start();
    }

    private void stopRecording() {
        isRecording = false;
        if (audioRecord != null) {
            audioRecord.stop();
            audioRecord.release();
            audioRecord = null; // Set to null after release to avoid double release
        }
    }

}
