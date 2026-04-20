//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.example.asr_llm_tts.tts_manager;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.RequiresApi;

import com.qualcomm.qti.voice.assist.tts.sdk.TTS;
import com.qualcomm.qti.voice.assist.tts.sdk.TTSResultCallback;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class TTSManager {
    private static final String TAG = "TTSManager";

    private TTS mTTS;

    private Context mContext;
    private Handler mIOHandler;
    private HandlerThread mIOThread;
    private AudioManager mAudioManager;

    private Runnable mOnFinishedRunnable;
    private AudioTrack mAudioTrack;
    private volatile boolean isReceivingData;
    private volatile int mCurrentAudioTrackSessionId;
    private volatile String mCurrentFilePath;
    private SimpleDateFormat mSimpleDateFormat = new SimpleDateFormat("yyyyMMdd_HHmmssSSS",
            Locale.getDefault());

    public TTSManager(Context context) {
        mContext = context;
        mAudioManager = context.getSystemService(AudioManager.class);
        mIOThread = new HandlerThread("TTS_IO_Thread");
        mIOThread.start();
        mIOHandler = new Handler(mIOThread.getLooper());
        mTTS = TTS.newInstance(new TTSResultCallback() {
            @Override
            public void onStart(int sampleRateInHz, int audioFormat, int channelCount) {
                Log.d(TAG, "onStart, sampleRate=" + sampleRateInHz +
                        ", audioFormat=" + audioFormat + ", channelCount=" + channelCount);
                mIOHandler.post(() -> {
                    isReceivingData = true;
                    if (mCurrentAudioTrackSessionId != 0) {
                        mAudioTrack = buildAudioTrack(sampleRateInHz, audioFormat);
                        mAudioTrack.play();
                    }
                });
            }

            @RequiresApi(api = Build.VERSION_CODES.O)
            @Override
            public void onAudioAvailable(byte[] buff, int offset, int size) {
                Log.d(TAG, "onAudioAvailable");
                writeToAudioTrack(buff, offset, size);
                saveToPCMFile(buff, offset, size);
            }

            @RequiresApi(api = Build.VERSION_CODES.O)
            @Override
            public void onDone() {
                Log.d(TAG, "onDone() mOnFinishedRunnable=" + mOnFinishedRunnable);
                isReceivingData = false;
                releaseAudioTrack();
                replaceWithWavFile();
                notifyOnFinished();
            }

            @Override
            public void onError(int errorCode) {
                Log.d(TAG, "onError (" + errorCode + ")");
                isReceivingData = false;
                releaseAudioTrack();
                notifyOnFinished();
            }
        });
    }

    public void updateTTSConfig(String lang, String model_path, String audioEncoding,
                                String speechRate, String pitch,
                                String volumeGain, String sampleRate) {
        mTTS.setLanguage(lang);
        mTTS.setModelPath(model_path);
        mTTS.setAudioEncoding(audioEncoding);
        mTTS.setSpeechRate(speechRate);
        mTTS.setPitch(pitch);
        mTTS.setVolumeGain(volumeGain);
        mTTS.setSampleRate(sampleRate);
    }

    public void init(String skelLibPath) {
        mTTS.setSkelLibPath(skelLibPath);
        mTTS.init();
    }

    private AudioTrack buildAudioTrack(int sampleRate, int format) {
        AudioAttributes audioAttributes = new AudioAttributes.Builder()
                .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
                .setUsage(AudioAttributes.USAGE_MEDIA)
                .build();
        AudioFormat audioFormat = new AudioFormat.Builder()
                .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
                .setSampleRate(sampleRate)
                .build();
        AudioTrack.Builder builder = new AudioTrack.Builder();
        int minBufferSize = AudioTrack.getMinBufferSize(sampleRate,
                AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);
        builder.setAudioAttributes(audioAttributes)
                .setAudioFormat(audioFormat)
                .setSessionId(mCurrentAudioTrackSessionId)
                .setBufferSizeInBytes(minBufferSize * 4)
                .setTransferMode(AudioTrack.MODE_STREAM);
        return builder.build();
    }

    private void releaseAudioTrack() {
        mIOHandler.post(() -> {
            Log.d(TAG, "try releaseAudioTrack");
            if (mCurrentAudioTrackSessionId != 0) {
                if (mAudioTrack != null) {
                    stopAudioTrack(false);
                    mAudioTrack.release();
                    Log.d(TAG, "mAudioTrack.release();");
                    mAudioTrack = null;
                }
                mCurrentAudioTrackSessionId = 0;
            }
        });

    }

    private void stopAudioTrack(boolean immediate) {
        if (mAudioTrack != null && mAudioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
            Log.d(TAG, "stopAudioTrack " + (immediate? "immediately":""));
            if (immediate) {
                try {
                    mAudioTrack.pause();
                    mAudioTrack.flush();
                } catch (IllegalStateException e) {
                    Log.w(TAG, "pause AudioTrack error", e);
                }
            } else {
                try {
                    Log.d(TAG, "About to sleep 460ms for audiotrack play completed");
                    Thread.sleep(460);
                } catch (InterruptedException e) {
                    Log.w(TAG, "sleep interrupted");
                }
                mAudioTrack.stop();
            }
        }
    }

    private void writeToAudioTrack(byte[] buff, int offset, int size) {
        mIOHandler.post(() -> {
            if (mCurrentAudioTrackSessionId != 0 && mAudioTrack != null) {
                if (mAudioTrack.write(buff, offset, size) != size) {
                    Log.e(TAG, "play(): Could not write all the samples to the audio device !");
                }
                Log.d(TAG, "writeToAudioTrack");
            }
        });
    }

    @RequiresApi(api = Build.VERSION_CODES.O)
    private void saveToPCMFile(byte[] data, int offset, int size) {
        mIOHandler.post(() -> {
            if (mCurrentFilePath != null) {
                try {
                    File pcmFile = new File(mCurrentFilePath);
                    byte[] realData = new byte[size];
                    System.arraycopy(data, offset, realData, 0, size);
                    Files.write(pcmFile.toPath(), realData,
                            StandardOpenOption.APPEND, StandardOpenOption.CREATE);
                } catch (IOException e) {
                    Log.e(TAG, "saveToPCMFile error", e);
                }
            }
        });

    }

    @RequiresApi(api = Build.VERSION_CODES.O)
    private void replaceWithWavFile() {
        mIOHandler.post(() -> {
            if (mCurrentFilePath != null) {
                try {
                    String wavFilePath = mCurrentFilePath.replace("pcm", "wav");
                    long bytes = Files.size(Paths.get(mCurrentFilePath));
                    byte[] header = getWavHeader(bytes, Integer.parseInt(mTTS.getSampleRate()));
                    Path wavPath = Paths.get(wavFilePath);
                    Files.write(wavPath, header, StandardOpenOption.CREATE);
                    Files.write(wavPath, Files.readAllBytes(Paths.get(mCurrentFilePath)),
                            StandardOpenOption.APPEND);
                    Files.delete(Paths.get(mCurrentFilePath));
                    Toast.makeText(mContext, "Saved in " + wavFilePath, Toast.LENGTH_SHORT)
                            .show();
                } catch (IOException e) {
                    Log.e(TAG, "replaceWithWavFile error", e);
                }
            }
        });
    }

    public void speak(String text, Runnable onFinished) {
        mOnFinishedRunnable = onFinished;
        mCurrentAudioTrackSessionId = mAudioManager.generateAudioSessionId();
        mTTS.start(text);
    }

    public void saveToFile(String text, Runnable onFinished) {

        mOnFinishedRunnable = onFinished;

        String datetime = mSimpleDateFormat.format(new Date());
        Path sampleDir = null;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            sampleDir = mContext.getExternalFilesDir("").toPath().resolve("tts_samples");
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            if (!sampleDir.toFile().exists()) {
                try {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                        Files.createDirectories(sampleDir);
                    }
                } catch (IOException e) {
                    throw new RuntimeException(e);
                }
            }
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            mCurrentFilePath = sampleDir.resolve(datetime + "_tts.pcm").toString();
        }
        mTTS.start(text);
    }

    public void stop() {
        mIOHandler.removeCallbacksAndMessages(null);
        stopAudioTrack(true);
        mTTS.stop();
        if (!isReceivingData) {
            releaseAudioTrack();
            notifyOnFinished();
        }
    }

    public void deInit() {
        mTTS.stop();
        mTTS.deInit();
    }

    public void release() {
        stop();
        mTTS.deInit();
        mTTS.release();
        mIOThread.quitSafely();
    }

    private void notifyOnFinished() {
        mIOHandler.post(() -> {
            if (mOnFinishedRunnable != null) {
                mOnFinishedRunnable.run();
            }
            mOnFinishedRunnable = null;
            mCurrentFilePath = null;
        });
    }

    private static byte[] getWavHeader(long audioDataLength, int pSampleRate) {
        long sampleRate = pSampleRate;
        int BITS_PER_SAMPLE = 16;
        long NUM_CHANNELS = 1;
        long audioDataAndHeaderLength = 44 + audioDataLength;
        long blocksPerSecond = sampleRate;
        long dataRate = BITS_PER_SAMPLE * sampleRate * NUM_CHANNELS / 8;
        byte[] wavHeader = new byte[44];
        wavHeader[0] = 'R';  // chunk ID = "RIFF"
        wavHeader[1] = 'I';
        wavHeader[2] = 'F';
        wavHeader[3] = 'F';
        wavHeader[4] = (byte) (audioDataAndHeaderLength & 0xff); // chunk size
        wavHeader[5] = (byte) ((audioDataAndHeaderLength >> 8) & 0xff);
        wavHeader[6] = (byte) ((audioDataAndHeaderLength >> 16) & 0xff);
        wavHeader[7] = (byte) ((audioDataAndHeaderLength >> 24) & 0xff);
        wavHeader[8] = 'W'; // wave ID = "WAVE"
        wavHeader[9] = 'A';
        wavHeader[10] = 'V';
        wavHeader[11] = 'E';
        wavHeader[12] = 'f'; // chunk ID = "fmt "
        wavHeader[13] = 'm';
        wavHeader[14] = 't';
        wavHeader[15] = ' ';
        wavHeader[16] = 16;  // chunk size = 16
        wavHeader[17] = 0;
        wavHeader[18] = 0;
        wavHeader[19] = 0;
        wavHeader[20] = 1; // format code (0x0001 is PCM)
        wavHeader[21] = 0;
        wavHeader[22] = (byte) NUM_CHANNELS;  // number of interleaved channels
        wavHeader[23] = 0;
        wavHeader[24] = (byte) (blocksPerSecond & 0xff); // Sampling rate (blocks/sec)
        wavHeader[25] = (byte) ((blocksPerSecond >> 8) & 0xff);
        wavHeader[26] = (byte) ((blocksPerSecond >> 16) & 0xff);
        wavHeader[27] = (byte) ((blocksPerSecond >> 24) & 0xff);
        wavHeader[28] = (byte) (dataRate & 0xff); // Data rate
        wavHeader[29] = (byte) ((dataRate >> 8) & 0xff);
        wavHeader[30] = (byte) ((dataRate >> 16) & 0xff);
        wavHeader[31] = (byte) ((dataRate >> 24) & 0xff);
        wavHeader[32] = (byte) (NUM_CHANNELS * BITS_PER_SAMPLE / 8); // Data block size
        wavHeader[33] = 0;
        wavHeader[34] = (byte) BITS_PER_SAMPLE;
        wavHeader[35] = 0;
        wavHeader[36] = 'd'; // chunk ID = "data"
        wavHeader[37] = 'a';
        wavHeader[38] = 't';
        wavHeader[39] = 'a';
        wavHeader[40] = (byte) (audioDataLength & 0xff);
        wavHeader[41] = (byte) ((audioDataLength >> 8) & 0xff);
        wavHeader[42] = (byte) ((audioDataLength >> 16) & 0xff);
        wavHeader[43] = (byte) ((audioDataLength >> 24) & 0xff);

        return wavHeader;
    }

}
