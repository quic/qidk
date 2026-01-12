//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.qualcomm.qti.speech_to_image

import android.annotation.SuppressLint
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.io.RandomAccessFile

object WAV {
    var audioRecorder: AudioRecord? = null
    var recordingThread: Thread? = null

    @SuppressLint("MissingPermission")
    fun startRecordingWAV(filepath: String){
        val bufferSize = AudioRecord.getMinBufferSize(
            16000,
            AudioFormat.CHANNEL_IN_MONO,
            AudioFormat.ENCODING_PCM_16BIT
        )

        audioRecorder = AudioRecord(
            MediaRecorder.AudioSource.MIC,
            16000,
            AudioFormat.CHANNEL_IN_MONO,
            AudioFormat.ENCODING_PCM_16BIT,
            bufferSize
        )

        audioRecorder?.startRecording()

        recordingThread = Thread {
            WAV.writeAudioDataToFile(audioRecorder, File(filepath), bufferSize)
        }.apply { start() }
    }

    fun stopRecordingWAV(){
        audioRecorder?.apply {
            stop()
            release()
        }
        audioRecorder = null
        recordingThread = null
    }

    fun writeAudioDataToFile(audioRecorder: AudioRecord?, outputFile: File, bufferSize: Int) {
        val data = ByteArray(bufferSize)
        FileOutputStream(outputFile).use { fos ->
            while (audioRecorder?.recordingState == AudioRecord.RECORDSTATE_RECORDING) {
                val read = audioRecorder?.read(data, 0, data.size) ?: 0
                if (read > 0) {
                    fos.write(data, 0, read)
                }
            }
        }
        addWavHeader(outputFile)
    }

    fun addWavHeader(outputFile: File) {
        val totalAudioLen = outputFile.length()
        val totalDataLen = totalAudioLen + 36
        val sampleRate = 16000
        val channels = 1
        val byteRate = 16 * sampleRate * channels / 8

        val header = byteArrayOf(
            'R'.code.toByte(), 'I'.code.toByte(), 'F'.code.toByte(), 'F'.code.toByte(),
            (totalDataLen and 0xff).toByte(),
            ((totalDataLen shr 8) and 0xff).toByte(),
            ((totalDataLen shr 16) and 0xff).toByte(),
            ((totalDataLen shr 24) and 0xff).toByte(),
            'W'.code.toByte(), 'A'.code.toByte(), 'V'.code.toByte(), 'E'.code.toByte(),
            'f'.code.toByte(), 'm'.code.toByte(), 't'.code.toByte(), ' '.code.toByte(),
            16, 0, 0, 0, 1, 0, channels.toByte(), 0,
            (sampleRate and 0xff).toByte(),
            ((sampleRate shr 8) and 0xff).toByte(),
            ((sampleRate shr 16) and 0xff).toByte(),
            ((sampleRate shr 24) and 0xff).toByte(),
            (byteRate and 0xff).toByte(),
            ((byteRate shr 8) and 0xff).toByte(),
            ((byteRate shr 16) and 0xff).toByte(),
            ((byteRate shr 24) and 0xff).toByte(),
            (2 * 16 / 8).toByte(), 0, 16, 0,
            'd'.code.toByte(), 'a'.code.toByte(), 't'.code.toByte(), 'a'.code.toByte(),
            (totalAudioLen and 0xff).toByte(),
            ((totalAudioLen shr 8) and 0xff).toByte(),
            ((totalAudioLen shr 16) and 0xff).toByte(),
            ((totalAudioLen shr 24) and 0xff).toByte()
        )

        try {
            val raf = RandomAccessFile(outputFile, "rw")
            raf.seek(0)
            raf.write(header, 0, 44)
            raf.close()
        } catch (e: IOException) {
            e.printStackTrace()
        }
    }
}