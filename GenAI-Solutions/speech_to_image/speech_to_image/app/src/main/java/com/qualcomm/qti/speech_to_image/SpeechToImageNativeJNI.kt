//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.qualcomm.qti.speech_to_image

import android.util.Log

object SpeechToImageNativeJNI {
    init {
        System.loadLibrary("speech_to_image-native")
    }

    fun generateImage(prompt: String, fileName: String) : Boolean {
        val startTime = System.nanoTime()

        val tokensStr = tokenize(prompt);

        if (tokensStr.isEmpty()) {
            Log.e("StableDiff", "Error in tokenizer: void string")
            return false;
        }

        val tokensSplit =
            tokensStr.split(",").map { it.toLong() }.toLongArray()
        val tokens = tokensSplit.copyOf(77).apply {
            fill(49407, tokensSplit.size, 77)
        }

        runStableDiffusion(tokens, fileName)

        val endTime = System.nanoTime()
        val elapsedTime = (endTime - startTime) / 1_000_000
        Log.i("StableDiff", "Elapsed time: $elapsedTime ms")
        return true;
    }


    fun speechToText(audioPath: String): String {
        return detokenize(runWhisper(audioPath));
    }

    external fun initializeTokenizer(path: String): Int
    external fun initializeDetokenizer(path: String): Int
    private external fun tokenize(text: String): String
    private external fun detokenize(ids: String): String
    external fun freeTokenizer()
    external fun initializeApp(cacheDir: String) : Int
    private external fun runStableDiffusion(condTokens : LongArray, fileName: String)
    private external fun runWhisper(audioPath: String): String;
    external fun freeApp(): Int
    external fun stopStableDiffusion();
}