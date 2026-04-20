//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include <iostream>
#include <vector>
#include "log_mel_spectrogram.hpp"
#include "preprocess.hpp"

std::vector<float> log_mel(std::string audio_path, std::string cache_dir){
    mel_spectrogram::LogMelSpectrogram lms = mel_spectrogram::LogMelSpectrogram(cache_dir + "/mel_80.bin");

    return lms.load_wav_audio_and_compute(audio_path);
}
