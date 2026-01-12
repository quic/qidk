//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#pragma once

#include <memory>
#include <queue>
#include <random>

#include "IOTensor.hpp"
#include "SpeechToImageApp.hpp"

extern bool stop_stable_diffusion;

namespace qnn {
namespace tools {
namespace speech_to_image {

enum class StatusCode {
  SUCCESS,
  FAILURE,
  FAILURE_INPUT_LIST_EXHAUSTED,
  FAILURE_SYSTEM_ERROR,
  FAILURE_SYSTEM_COMMUNICATION_ERROR,
  QNN_FEATURE_UNSUPPORTED
};


class QnnSpeechToImage {
 public:
  QnnSpeechToImage(QnnFunctionPointers qnnFunctionPointers,
               void *backendHandle
            );

  // @brief Print a message to STDERR then return a nonzero
  //  exit status.
  int32_t reportError(const std::string &err);

  StatusCode initialize();

  StatusCode initializeBackend();

  StatusCode createFromBinary(std::vector<std::string> listOfModels);

  void initializeTensorsStableDiffusion();

  void initializeTensorsWhisper();

  void runTextEncoder();

  void executeLoop();

  void runWhisperEncoder();

  std::string runWhisperDecoder();

  StatusCode finalizeGraphs();

  StatusCode freeContext();

  StatusCode terminateBackend();

  StatusCode freeGraphs();

  std::string getBackendBuildId();

  StatusCode isDevicePropertySupported();

  StatusCode createDevice();

  StatusCode freeDevice();

  StatusCode verifyFailReturnStatus(Qnn_ErrorHandle_t errCode);

  virtual ~QnnSpeechToImage();

 private:

  QnnFunctionPointers m_qnnFunctionPointers;
  iotensor::InputDataType m_inputDataType;

  QnnBackend_Config_t **m_backendConfig = nullptr;
  Qnn_ContextHandle_t m_context[5];
  QnnContext_Config_t **m_contextConfig[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
  bool m_debug;
  qnn_wrapper_api::GraphInfo_t **m_graphsInfo;
  uint32_t m_graphsCount;
  void *m_backendLibraryHandle;
  iotensor::IOTensor m_ioTensor;
  bool m_isBackendInitialized;
  bool m_isContextCreated;
  Qnn_ProfileHandle_t m_profileBackendHandle              = nullptr;
  qnn_wrapper_api::GraphConfigInfo_t **m_graphConfigsInfo = nullptr;
  uint32_t m_graphConfigsInfoCount;
  Qnn_LogHandle_t m_logHandle         = nullptr;
  Qnn_BackendHandle_t m_backendHandle = nullptr;
  Qnn_DeviceHandle_t m_deviceHandle   = nullptr;

  Qnn_Tensor_t* input_tensors[5]  = {nullptr, nullptr, nullptr, nullptr, nullptr};
  Qnn_Tensor_t* output_tensors[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};

  Qnn_ClientBuffer_t uncondBuffer_text_enc;
  Qnn_ClientBuffer_t condBuffer_text_enc;
  Qnn_ClientBuffer_t uncondBuffer_noise_predict;

};
}  // namespace speech_to_image
}  // namespace tools
}  // namespace qnn
