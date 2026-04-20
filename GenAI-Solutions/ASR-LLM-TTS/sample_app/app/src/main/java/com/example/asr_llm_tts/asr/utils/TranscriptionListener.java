//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

package com.example.asr_llm_tts.asr.utils;


// TranscriptionListener.java
public interface TranscriptionListener {
    void onTranscriptionFinal(String fullText, String language);
    void onTranscriptionPartial(String partialText, String language);
}
