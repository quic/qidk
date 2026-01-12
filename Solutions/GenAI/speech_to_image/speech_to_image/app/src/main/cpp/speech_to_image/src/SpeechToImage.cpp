//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#include <iostream>
#include <memory>
#include <string>
#include <string.h>
#include <fstream>
#include <iostream>

#include "BuildId.hpp"
#include "DynamicLoadUtil.hpp"
#include "Logger.hpp"
#include "DynamicLoading.hpp"
#include "GetOpt.hpp"
#include "QnnSpeechToImage.hpp"
#include "QnnSpeechToImageUtils.hpp"
#include "DataUtil.hpp"
#include "AndroidLogger.hpp"
#include "SpeechToImage.hpp"

static void* sg_backendHandle{nullptr};
std::string assets_path;

using namespace qnn::tools;

std::unique_ptr<speech_to_image::QnnSpeechToImage> app;
std::vector<int32_t> tokensId;
std::string audio_path;
std::string cache_path;
std::string name_png = "output.png";

void run_stable_diffusion(std::vector<int32_t> cond_tokens, std::string file_name){
  tokensId = cond_tokens;
  name_png = file_name;

  app->initializeTensorsStableDiffusion();
  app->runTextEncoder();
  app->executeLoop();
}

std::string run_whisper(std::string path){
  audio_path = path;
  app->initializeTensorsWhisper();
  app->runWhisperEncoder();
  return app->runWhisperDecoder();
}

int free_app(){
  if (speech_to_image::StatusCode::SUCCESS != app->freeContext()) {
    return app->reportError("Context Free failure");
  }


  auto freeDeviceStatus = app->freeDevice();
  if (speech_to_image::StatusCode::SUCCESS != freeDeviceStatus) {
    return app->reportError("Device Free failure");
  }

  if (sg_backendHandle) {
    pal::dynamicloading::dlClose(sg_backendHandle);
  }

  return EXIT_SUCCESS;
}

int initialize(std::string dirpath) {
  assets_path = dirpath;
  cache_path = dirpath;

  std::string dspBackEndPath = "libQnnHtp.so";
  std::string systemLibraryPath = "libQnnSystem.so";
  std::vector<std::string> models{assets_path + "text_encoder.bin",
                                  assets_path + "unet.bin",
                                  assets_path + "vae_decoder.bin",
                                  assets_path + "whisper_encoder_tiny.bin",
                                  assets_path + "whisper_decoder_tiny.bin"};


  if (!qnn::log::initializeLogging()) {
    std::cerr << "ERROR: Unable to initialize logging!\n";
    return EXIT_FAILURE;
  }

  {
    qnn::tools::speech_to_image::QnnFunctionPointers qnnFunctionPointers;
    // Load backend and model .so and validate all the required function symbols are resolved
    auto statusCode = dynamicloadutil::getQnnFunctionPointers(dspBackEndPath,
                                                              &qnnFunctionPointers,
                                                              &sg_backendHandle);
    if (dynamicloadutil::StatusCode::SUCCESS != statusCode) {
      if (dynamicloadutil::StatusCode::FAIL_LOAD_BACKEND == statusCode) {
        qnn::tools::speech_to_image::exitWithMessage(
                "Error initializing QNN Function Pointers: could not load backend: " +
                dspBackEndPath,
                EXIT_FAILURE);
      } else {
        qnn::tools::speech_to_image::exitWithMessage("Error initializing QNN Function Pointers",
                                                EXIT_FAILURE);
      }
    }

    statusCode = dynamicloadutil::getQnnSystemFunctionPointers(systemLibraryPath,
                                                               &qnnFunctionPointers);
    if (dynamicloadutil::StatusCode::SUCCESS != statusCode) {
      qnn::tools::speech_to_image::exitWithMessage("Error initializing QNN System Function Pointers",
                                              EXIT_FAILURE);
    }

    app = std::make_unique<speech_to_image::QnnSpeechToImage>(qnnFunctionPointers, sg_backendHandle);

    if (nullptr == app) {
      return EXIT_FAILURE;
    }

    if (speech_to_image::StatusCode::SUCCESS != app->initialize()) {
      return app->reportError("Initialization failure");
    }

    if (speech_to_image::StatusCode::SUCCESS != app->initializeBackend()) {
      return app->reportError("Backend Initialization failure");
    }

    LOGI("creating device: ");

    auto devicePropertySupportStatus = app->isDevicePropertySupported();
    if (speech_to_image::StatusCode::FAILURE != devicePropertySupportStatus) {
      auto createDeviceStatus = app->createDevice();
      if (speech_to_image::StatusCode::SUCCESS != createDeviceStatus) {
        return app->reportError("Device Creation failure");
      }
    }

    LOGI("Device Created!");

    if (speech_to_image::StatusCode::SUCCESS != app->createFromBinary(models)) {
      return app->reportError("Create From Binary failure");
    }
    return EXIT_SUCCESS;
  }
}


