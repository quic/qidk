//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.example.asr_llm_tts.asr.utils;

import android.annotation.SuppressLint;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;

/**
 * The Simple audio recorder as InputStream.
 */
public class SimpleAudioRecord extends InputStream {
    private final static String TAG = SimpleAudioRecord.class.getSimpleName();
    private static int DEFAULT_SAMPLE_RATE = 16000;
    private static int DEFAULT_AUDIO_SOURCE = MediaRecorder.AudioSource.DEFAULT;
    private AudioRecord mAudioRecord;

    /**
     * The enum State.
     */
    public enum State {
        /**
         * Idle state.
         */
        IDLE,
        /**
         * Started state.
         */
        STARTED,
        /**
         * Stopped state.
         */
        STOPPED
    }

    private State mState = State.IDLE;

    /**
     * The M data.
     */
    byte[] mData;


    /**
     * Instantiates a new Simple audio record.
     */
    public SimpleAudioRecord() {
    }

    @Override
    public int read() {
        throw new UnsupportedOperationException("This operation is not supported");
    }

    private void startAudioRecord() {
        Log.d(TAG, "startAudioRecord");
        try {
            mAudioRecord = build();
            mAudioRecord.startRecording();
            setState(State.STARTED);

            if (mAudioRecord.getRecordingState() != AudioRecord.RECORDSTATE_RECORDING) {
                Log.e(TAG, "Unable to initialize the AudioRecord instance");
                mAudioRecord.stop();
                setState(State.STOPPED);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void setState(State state) {
        mState = state;
    }

    @Override
    public synchronized int read(byte[] buffer, int byteOffset, int byteCount) throws IOException {
        //Log.d(TAG, "read() byteCount=" + byteCount + ", byteOffset=" + byteOffset);
        if (mState == State.IDLE) {
            startAudioRecord();
        } else if (mState == State.STOPPED) {
            return 0;
        }

        if (mData.length != byteCount) {
            mData = new byte[byteCount];
        }

        int size = mAudioRecord.read(mData, byteOffset, byteCount, AudioRecord.READ_BLOCKING);
        System.arraycopy(mData, 0, buffer, 0, mData.length);

        return size;
    }

    @Override
    public synchronized void close() throws IOException {
        releaseAudioRecord();
    }

    /**
     * Gets state.
     *
     * @return the state
     */
    public State getState() {
        return mState;
    }

    private AudioRecord build() throws RuntimeException {
        final int minBufferSize = AudioRecord.getMinBufferSize(
                DEFAULT_SAMPLE_RATE,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT
        );

        mData = new byte[minBufferSize];

        if (minBufferSize == AudioRecord.ERROR_BAD_VALUE || minBufferSize == AudioRecord.ERROR) {
            throw new RuntimeException("AudioRecord invalid bufferSize=" + minBufferSize + ", sampleRate=" + DEFAULT_SAMPLE_RATE);
        }
        // Permission is checked in the Activity
        @SuppressLint("MissingPermission") final AudioRecord audioRecord = new AudioRecord(
                DEFAULT_AUDIO_SOURCE,
                DEFAULT_SAMPLE_RATE,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                minBufferSize
        );

        if (audioRecord == null
                || audioRecord.getState() != AudioRecord.STATE_INITIALIZED
                || audioRecord.getRecordingState() != AudioRecord.RECORDSTATE_STOPPED) {
            throw new RuntimeException("Error building AudioRecord sampleRate=" + DEFAULT_SAMPLE_RATE + ", bufferSize=" + minBufferSize);
        }

        return audioRecord;
    }


    private void releaseAudioRecord() {
        Log.d(TAG, "SimpleAudioRecording releaseAudioRecord()");
        if (mAudioRecord == null) {
            setState(State.STOPPED);
            return;
        }

        if (mState == State.STARTED) {
            try {
                mAudioRecord.stop();
            } catch (IllegalStateException ex) {
                // Swallow it, main goal is to stop all recording.
            }
        }
        mAudioRecord.release();
        mAudioRecord = null;
        setState(State.IDLE);
    }
}
