//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.example.asr_llm_tts.asr;

import android.content.Context;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.StyleSpan;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.Nullable;

import com.example.asr_llm_tts.asr.utils.SimpleAudioRecord;
import com.example.asr_llm_tts.asr.utils.TranscriptionListener;
import com.qualcomm.qti.voice.assist.whisper.sdk.Whisper;
import com.qualcomm.qti.voice.assist.whisper.sdk.WhisperResponseListener;

import java.io.File;
import java.util.List;

public class Whisper_ASR implements  WhisperResponseListener {

    private final String DEBUG_TAG="Whisper_DEBUG_TAG";
    private final String INFO_TAG="Whisper_INFO_TAG";
    private String MODELS_PATH = "/storage/emulated/0/Android/data/com.example.tts/files/whisper_small";

    private Whisper whisper;
    private SimpleAudioRecord mSimpleAudioRecord;
    private boolean isTranscribeFile;
    private boolean mRecording;
    private String mLastLanguage = null;

    private String mTranscription = "";
    private final Context appContext;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    private @Nullable TranscriptionListener listener;


    public Whisper_ASR( Context context,@Nullable TranscriptionListener listener ){
        this.appContext = context.getApplicationContext();
        this.listener = listener;

    }
    public void initializeWhisper() {
        Log.d(INFO_TAG, "initializeWhisper()");

        // Create a default Whisper instance
        whisper = Whisper.newInstance();
        Log.d(INFO_TAG, "Whisper SDK: " + whisper.getVersion());

        // Initialize whisper library
        int res = whisper.init(MODELS_PATH);

        // Set debug directory
        File dir = new File(appContext.getExternalFilesDir(null) + "/recordings");
        whisper.setDebugDir(dir);

        // Defaults
        whisper.enablePartialTranscriptions(true);
        whisper.enableContinuousTranscription(false);

        Log.d(INFO_TAG, whisper.toString());

        if (res != Whisper.NO_ERROR) {
            //Toast.makeText(appContext, "Error initializing whisper!", Toast.LENGTH_SHORT).show();
            Log.e(INFO_TAG, "\"Error initializing whisper! ");
        }

        // Setup listener
        whisper.setListener(this);

        List<Whisper.ChineseConvertType> types = whisper.getSupportedConvertTypes();
        Log.d(INFO_TAG, "Supported Chinese Convert Types: " + types);
    }

    @Override
    public void onTranscription(String transcription, String language, boolean isFinal, int code, Bundle results) {
        Log.d(INFO_TAG, "onTranscription: " + transcription + ", isFinal: " + isFinal);


        if (isFinal) {
            // Display language tag
            if (mLastLanguage == null || !language.equals(mLastLanguage)) {
                mLastLanguage = language;
            }

            // Save all finalized transcriptions
            mTranscription += transcription + System.lineSeparator();

            if (listener != null) {
                mainHandler.post(() -> listener.onTranscriptionFinal(mTranscription, language));
            }

            Log.d(DEBUG_TAG,"MTranscription:"+mTranscription);

        } else {
            // Show partial but do not save transcription
            SpannableString partial = new SpannableString(transcription);
            partial.setSpan(new StyleSpan(Typeface.ITALIC), 0, transcription.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
            Log.d(DEBUG_TAG,"MTranscription:"+mTranscription);

            if (listener != null) {
                mainHandler.post(() -> listener.onTranscriptionPartial(transcription, language));
            }

        }

    }

    @Override
    public void onError(int i, Exception e) {
        Log.d(DEBUG_TAG, "__ON_ERROR__");
    }

    @Override
    public void onRecordingStopped() {
        Log.d(DEBUG_TAG, "__ON_RECORDING_STOPPED__");
    }

    @Override
    public void onSpeechStart() {
        Log.d(DEBUG_TAG, "__ON_SPEECH_START__");
    }

    @Override
    public void onSpeechEnd() {
        Log.d(DEBUG_TAG, "__ON_SPEECH_END__");
    }

    @Override
    public void onFinished() {
        stopRecording();
        stopTranscribeFile();
        Log.d(DEBUG_TAG, "__ON_FINISHED__");
    }
    public void startRecording() {
        // Clear old
        mTranscription = "";
        mLastLanguage = null;

        // Start Whisper
        Log.d(DEBUG_TAG, "start Recording");
        mSimpleAudioRecord = new SimpleAudioRecord();
        whisper.start(mSimpleAudioRecord);
        mRecording = true;
    }
    private void stopTranscribeFile() {
        if (isTranscribeFile) {
            Log.d(DEBUG_TAG, "stop Transcribe file");
            whisper.stop();
            isTranscribeFile = false;
        }

    }
    public void stopRecording() {
        // Stop Whisper
        if (mRecording) {
            Log.d(DEBUG_TAG, "stop Recording");
            whisper.stop();
            mSimpleAudioRecord = null;
            mRecording = false;
        }

    }
}
