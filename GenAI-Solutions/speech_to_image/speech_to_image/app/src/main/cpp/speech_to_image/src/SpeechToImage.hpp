//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#ifndef SPEECH_TO_IMAGE_HPP
#define SPEECH_TO_IMAGE_HPP
#include <string>

extern std::vector<int32_t> tokensId;
extern std::string name_png;
extern std::string assets_path;
extern std::string audio_path;
extern std::string cache_path;
char* tokenize(char* text, char* tokenizer_path);
int initialize(std::string);
void run_stable_diffusion(std::vector<int32_t>, std::string);
std::string run_whisper(std::string path);
int free_app();

#endif //SPEECH_TO_IMAGE_HPP
