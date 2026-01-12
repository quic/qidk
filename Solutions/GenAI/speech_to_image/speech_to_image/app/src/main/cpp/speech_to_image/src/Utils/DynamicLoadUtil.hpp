//==============================================================================
//
//  Copyright (c) 2025 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================


#pragma once

#include "SpeechToImageApp.hpp"

namespace qnn {
namespace tools {
namespace dynamicloadutil {
enum class StatusCode {
  SUCCESS,
  FAILURE,
  FAIL_LOAD_BACKEND,
  FAIL_LOAD_MODEL,
  FAIL_SYM_FUNCTION,
  FAIL_GET_INTERFACE_PROVIDERS,
  FAIL_LOAD_SYSTEM_LIB,
};

StatusCode getQnnFunctionPointers(std::string backendPath,
                                  speech_to_image::QnnFunctionPointers* qnnFunctionPointers,
                                  void** backendHandle);
StatusCode getQnnSystemFunctionPointers(std::string systemLibraryPath,
                                        speech_to_image::QnnFunctionPointers* qnnFunctionPointers);
}  // namespace dynamicloadutil
}  // namespace tools
}  // namespace qnn
