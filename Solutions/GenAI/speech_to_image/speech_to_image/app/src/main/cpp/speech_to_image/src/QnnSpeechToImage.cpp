//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================


#include <inttypes.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "DataUtil.hpp"
#include "Logger.hpp"
#include "PAL/Directory.hpp"
#include "PAL/FileOp.hpp"
#include "PAL/Path.hpp"
#include "PAL/StringOp.hpp"
#include "QnnSpeechToImage.hpp"
#include "QnnSpeechToImageUtils.hpp"
#include "QnnWrapperUtils.hpp"
#include "QnnTypeMacros.hpp"
#include "SpeechToImage.hpp"
#include "raw_image_to_png.hpp"
#include "whisper.hpp"
#include "float16.hpp"

bool stop_stable_diffusion = false;

using namespace qnn;
using namespace qnn::tools;

speech_to_image::QnnSpeechToImage::QnnSpeechToImage(QnnFunctionPointers qnnFunctionPointers,
                                       void* backendLibraryHandle
                                      )
    : m_qnnFunctionPointers(qnnFunctionPointers),
      m_backendLibraryHandle(backendLibraryHandle),
      m_isBackendInitialized(false),
      m_isContextCreated(false){
        datautil::initialize_scheduler();
  return;
}

speech_to_image::QnnSpeechToImage::~QnnSpeechToImage() {
  // Free context if not already done
  if (m_isContextCreated) {
    QNN_DEBUG("Freeing context");
    if (QNN_CONTEXT_NO_ERROR !=
        m_qnnFunctionPointers.qnnInterface.contextFree(m_context, nullptr)) {
      QNN_ERROR("Could not free context");
    }
  }
  m_isContextCreated = false;
  // Terminate backend
  if (m_isBackendInitialized && nullptr != m_qnnFunctionPointers.qnnInterface.backendFree) {
    QNN_DEBUG("Freeing backend");
    if (QNN_BACKEND_NO_ERROR != m_qnnFunctionPointers.qnnInterface.backendFree(m_backendHandle)) {
      QNN_ERROR("Could not free backend");
    }
  }
  m_isBackendInitialized = false;
  // Terminate logging in the backend
  if (nullptr != m_qnnFunctionPointers.qnnInterface.logFree && nullptr != m_logHandle) {
    if (QNN_SUCCESS != m_qnnFunctionPointers.qnnInterface.logFree(m_logHandle)) {
      QNN_WARN("Unable to terminate logging in the backend.");
    }
  }
  return;
}

std::string speech_to_image::QnnSpeechToImage::getBackendBuildId() {
  char* backendBuildId{nullptr};
  if (QNN_SUCCESS !=
      m_qnnFunctionPointers.qnnInterface.backendGetBuildId((const char**)&backendBuildId)) {
    QNN_ERROR("Unable to get build Id from the backend.");
  }
  return (backendBuildId == nullptr ? std::string("") : std::string(backendBuildId));
}

speech_to_image::StatusCode speech_to_image::QnnSpeechToImage::initialize() {
  // initialize logging in the backend
  if (log::isLogInitialized()) {
    auto logCallback = log::getLogCallback();
    auto logLevel    = log::getLogLevel();
    QNN_INFO("Initializing logging in the backend. Callback: [%p], Log Level: [%d]", logCallback, logLevel);
    if (QNN_SUCCESS != m_qnnFunctionPointers.qnnInterface.logCreate(logCallback, logLevel, &m_logHandle)) {
      QNN_WARN("Unable to initialize logging in the backend.");
    }
  } else {
    QNN_WARN("Logging not available in the backend.");
  }
  return StatusCode::SUCCESS;
}

// Simple method to report error from app to lib.
int32_t speech_to_image::QnnSpeechToImage::reportError(const std::string& err) {

  QNN_ERROR("%s", err.c_str());
  return EXIT_FAILURE;
}

// Initialize a QnnBackend.
speech_to_image::StatusCode speech_to_image::QnnSpeechToImage::initializeBackend() {
  auto qnnStatus = m_qnnFunctionPointers.qnnInterface.backendCreate(
      m_logHandle, (const QnnBackend_Config_t**)m_backendConfig, &m_backendHandle);
  if (QNN_BACKEND_NO_ERROR != qnnStatus) {
    QNN_ERROR("Could not initialize backend due to error = %d", qnnStatus);
    return StatusCode::FAILURE;
  }
  QNN_INFO("Initialize Backend Returned Status = %d", qnnStatus);
  m_isBackendInitialized = true;
  return StatusCode::SUCCESS;
}

// Terminate the backend after done.
speech_to_image::StatusCode speech_to_image::QnnSpeechToImage::terminateBackend() {
  if ((m_isBackendInitialized && nullptr != m_qnnFunctionPointers.qnnInterface.backendFree) &&
      QNN_BACKEND_NO_ERROR != m_qnnFunctionPointers.qnnInterface.backendFree(m_backendHandle)) {
    QNN_ERROR("Could not terminate backend");
    return StatusCode::FAILURE;
  }
  m_isBackendInitialized = false;
  return StatusCode::SUCCESS;
}

// Free context after done.
speech_to_image::StatusCode speech_to_image::QnnSpeechToImage::freeContext() {
  for(size_t i=0; i<3; i++){
    if (QNN_CONTEXT_NO_ERROR !=
      m_qnnFunctionPointers.qnnInterface.contextFree(m_context[i], m_profileBackendHandle)) {
      QNN_ERROR("Could not free context");
      return StatusCode::FAILURE;
    }
    m_isContextCreated = false;
  }
  
  return StatusCode::SUCCESS;
}

speech_to_image::StatusCode speech_to_image::QnnSpeechToImage::finalizeGraphs() {
  for (size_t graphIdx = 0; graphIdx < m_graphsCount; graphIdx++) {
    if (QNN_GRAPH_NO_ERROR !=
        m_qnnFunctionPointers.qnnInterface.graphFinalize(
            (*m_graphsInfo)[graphIdx].graph, m_profileBackendHandle, nullptr)) {
      return StatusCode::FAILURE;
    }
  }
  return StatusCode::SUCCESS;
}

speech_to_image::StatusCode speech_to_image::QnnSpeechToImage::createFromBinary(std::vector<std::string> listOfModels) {
  
  if (nullptr == m_qnnFunctionPointers.qnnSystemInterface.systemContextCreate ||
      nullptr == m_qnnFunctionPointers.qnnSystemInterface.systemContextGetBinaryInfo ||
      nullptr == m_qnnFunctionPointers.qnnSystemInterface.systemContextFree) {
    QNN_ERROR("QNN System function pointers are not populated.");
    return StatusCode::FAILURE;
  }

  uint64_t bufferSize[]{0, 0, 0, 0, 0};
  std::shared_ptr<uint8_t> buffer[]{nullptr, nullptr, nullptr, nullptr, nullptr};
  const QnnSystemContext_BinaryInfo_t* binaryInfo[]{nullptr, nullptr, nullptr, nullptr, nullptr};
  Qnn_ContextBinarySize_t binaryInfoSize[]{0, 0, 0, 0, 0};
  QnnSystemContext_Handle_t sysCtxHandle[]{nullptr, nullptr, nullptr, nullptr, nullptr};

  auto returnStatus = StatusCode::SUCCESS;
  for(size_t i=0; i<listOfModels.size(); i++){
    
    if (QNN_SUCCESS != m_qnnFunctionPointers.qnnSystemInterface.systemContextCreate(&sysCtxHandle[i])) {
      QNN_ERROR("Could not create system handle.");
      returnStatus = StatusCode::FAILURE;
    }
    // read serialized binary into a byte buffer
    tools::datautil::StatusCode status{tools::datautil::StatusCode::SUCCESS};
    std::tie(status, bufferSize[i]) = tools::datautil::getFileSize(listOfModels[i]);
    if (0 == bufferSize[i]) {
      QNN_ERROR("Received path to an empty file. Nothing to deserialize.");
      return StatusCode::FAILURE;
    }
    buffer[i] = std::shared_ptr<uint8_t>(new uint8_t[bufferSize[i]], std::default_delete<uint8_t[]>());
    if (!buffer[i]) {
      QNN_ERROR("Failed to allocate memory.");
      return StatusCode::FAILURE;
    }

    status = tools::datautil::readBinaryFromFile(
      listOfModels[i], reinterpret_cast<uint8_t*>(buffer[i].get()), bufferSize[i]);
    if (status != tools::datautil::StatusCode::SUCCESS) {
      QNN_ERROR("Failed to read binary data.");
      return StatusCode::FAILURE;
    }

    // inspect binary info
    if (StatusCode::SUCCESS == returnStatus &&
      QNN_SUCCESS != m_qnnFunctionPointers.qnnSystemInterface.systemContextGetBinaryInfo(
                         sysCtxHandle[i],
                         static_cast<void*>(buffer[i].get()),
                         bufferSize[i],
                         &binaryInfo[i],
                         &binaryInfoSize[i])) {
      QNN_ERROR("Failed to get context binary info");
      returnStatus = StatusCode::FAILURE;
    }
  }
  
  // fill GraphInfo_t based on binary info
  if (StatusCode::SUCCESS == returnStatus &&
    !copyMetadataToGraphsInfo(listOfModels.size(), binaryInfo, m_graphsInfo, m_graphsCount)) {
    QNN_ERROR("Failed to copy metadata.");
    returnStatus = StatusCode::FAILURE;
  }

  for(size_t i=0; i<listOfModels.size();i++){
    m_qnnFunctionPointers.qnnSystemInterface.systemContextFree(sysCtxHandle[i]);
    sysCtxHandle[i] = nullptr;
  }

  if (StatusCode::SUCCESS == returnStatus &&
    nullptr == m_qnnFunctionPointers.qnnInterface.contextCreateFromBinary) {
    QNN_ERROR("contextCreateFromBinaryFnHandle is nullptr.");
    returnStatus = StatusCode::FAILURE;
  }

  for(size_t i=0; i<listOfModels.size(); i++){
    if (StatusCode::SUCCESS == returnStatus &&
      m_qnnFunctionPointers.qnnInterface.contextCreateFromBinary(
        m_backendHandle,
        m_deviceHandle,
        (const QnnContext_Config_t**)m_contextConfig[i],
        static_cast<void*>(buffer[i].get()),
        bufferSize[i],
        &m_context[i],
        m_profileBackendHandle)) {
      QNN_ERROR("Could not create context from binary.");
      returnStatus = StatusCode::FAILURE;
    }
  }

  m_isContextCreated = true;
  if (StatusCode::SUCCESS == returnStatus) {
    for (size_t graphIdx = 0; graphIdx < m_graphsCount; graphIdx++) {
      if (nullptr == m_qnnFunctionPointers.qnnInterface.graphRetrieve) {
        QNN_ERROR("graphRetrieveFnHandle is nullptr.");
        returnStatus = StatusCode::FAILURE;
        break;
      }
      if (QNN_SUCCESS !=
        m_qnnFunctionPointers.qnnInterface.graphRetrieve(
            m_context[graphIdx], (*m_graphsInfo)[graphIdx].graphName, &((*m_graphsInfo)[graphIdx].graph))) {
        QNN_ERROR("Unable to retrieve graph handle for graph Idx: %d, graph name %s", graphIdx, (*m_graphsInfo)[graphIdx].graphName);
        returnStatus = StatusCode::FAILURE;
      }
    }
  }
  if (StatusCode::SUCCESS != returnStatus) {
    QNN_DEBUG("Cleaning up graph Info structures.");
    qnn_wrapper_api::freeGraphsInfo(&m_graphsInfo, m_graphsCount);
  }

  if(StatusCode::SUCCESS == returnStatus)
    QNN_INFO("speech_to_image -> Models loaded");

  return returnStatus;
}

speech_to_image::StatusCode speech_to_image::QnnSpeechToImage::verifyFailReturnStatus(Qnn_ErrorHandle_t errCode) {
  auto returnStatus = speech_to_image::StatusCode::FAILURE;
  switch (errCode) {
    case QNN_COMMON_ERROR_SYSTEM_COMMUNICATION:
      returnStatus = speech_to_image::StatusCode::FAILURE_SYSTEM_COMMUNICATION_ERROR;
      break;
    case QNN_COMMON_ERROR_SYSTEM:
      returnStatus = speech_to_image::StatusCode::FAILURE_SYSTEM_ERROR;
      break;
    case QNN_COMMON_ERROR_NOT_SUPPORTED:
      returnStatus = speech_to_image::StatusCode::QNN_FEATURE_UNSUPPORTED;
      break;
    default:
      break;
  }
  return returnStatus;
}

speech_to_image::StatusCode speech_to_image::QnnSpeechToImage::isDevicePropertySupported() {
  if (nullptr != m_qnnFunctionPointers.qnnInterface.propertyHasCapability) {
    auto qnnStatus =
        m_qnnFunctionPointers.qnnInterface.propertyHasCapability(QNN_PROPERTY_GROUP_DEVICE);
    if (QNN_PROPERTY_NOT_SUPPORTED == qnnStatus) {
      QNN_ERROR("Device property is not supported");
    }
    if (QNN_PROPERTY_ERROR_UNKNOWN_KEY == qnnStatus) {
      QNN_ERROR("Device property is not known to backend");
      return StatusCode::FAILURE;
    }
  }
  return StatusCode::SUCCESS;
}

speech_to_image::StatusCode speech_to_image::QnnSpeechToImage::createDevice() {
  if (nullptr != m_qnnFunctionPointers.qnnInterface.deviceCreate) {
      auto qnnStatus = m_qnnFunctionPointers.qnnInterface.deviceCreate(m_logHandle, nullptr, &m_deviceHandle);
    if (QNN_SUCCESS != qnnStatus && QNN_DEVICE_ERROR_UNSUPPORTED_FEATURE != qnnStatus) {
        QNN_ERROR("%d", qnnStatus);
      QNN_ERROR("Failed to create device");
      return verifyFailReturnStatus(qnnStatus);
    }
  }
  return StatusCode::SUCCESS;
}

speech_to_image::StatusCode speech_to_image::QnnSpeechToImage::freeDevice() {
  if (nullptr != m_qnnFunctionPointers.qnnInterface.deviceFree) {
    auto qnnStatus = m_qnnFunctionPointers.qnnInterface.deviceFree(m_deviceHandle);
    if (QNN_SUCCESS != qnnStatus && QNN_DEVICE_ERROR_UNSUPPORTED_FEATURE != qnnStatus) {
      QNN_ERROR("Failed to free device");
      return verifyFailReturnStatus(qnnStatus);
    }
  }
  return StatusCode::SUCCESS;
}

void speech_to_image::QnnSpeechToImage::initializeTensorsStableDiffusion() {
  for(size_t i=0; i<3; i++){
    QNN_DEBUG("Starting execution for graphIdx: %d", 0);
    auto graphInfo     = (*m_graphsInfo)[i];
    QNN_INFO("speech_to_image ->>>>>>>> Initializing tensor for Graph: %s", graphInfo.graphName);
    Qnn_Tensor_t* inputs  = nullptr;
    Qnn_Tensor_t* outputs = nullptr;
    if (iotensor::StatusCode::SUCCESS !=
        m_ioTensor.setupInputAndOutputTensors(&inputs, &outputs, (*m_graphsInfo)[i])) {
      QNN_ERROR("Error in setting up Input and output Tensors for graphIdx: %d", i);
    }
    iotensor::StatusCode iotReturnStatus;
    size_t numInputFilesPopulated;
    std::tie(iotReturnStatus, numInputFilesPopulated) = m_ioTensor.populateInputTensors(i, inputs, graphInfo, m_inputDataType);
    input_tensors[i] = inputs;
    output_tensors[i] = outputs;
  }
}


void speech_to_image::QnnSpeechToImage::initializeTensorsWhisper() {
  for(size_t i=3; i<5; i++){
    QNN_DEBUG("Starting execution for graphIdx: %d", i);
    auto graphInfo     = (*m_graphsInfo)[i];
    QNN_INFO("speech_to_image ->>>>>>>> Initializing tensor for Graph: %s", graphInfo.graphName);
    Qnn_Tensor_t* inputs  = nullptr;
    Qnn_Tensor_t* outputs = nullptr;
    if (iotensor::StatusCode::SUCCESS !=
        m_ioTensor.setupInputAndOutputTensors(&inputs, &outputs, (*m_graphsInfo)[i])) {
      QNN_ERROR("Error in setting up Input and output Tensors for graphIdx: %d", i);
    }
    iotensor::StatusCode iotReturnStatus;
    size_t numInputFilesPopulated;
    std::tie(iotReturnStatus, numInputFilesPopulated) = m_ioTensor.populateInputTensors(i, inputs, graphInfo, m_inputDataType);
    input_tensors[i] = inputs;
    output_tensors[i] = outputs;
  }
}

void speech_to_image::QnnSpeechToImage::runTextEncoder(){
    //Select the graph corresponding to the text encoder model for execution
    auto graphInfo = (*m_graphsInfo)[0];

    //Get the information of the output tensor (Required to configure the two buffers that will be used to keep the two outputs)
    size_t length{0};
    std::vector<size_t> dims;
    m_ioTensor.fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(output_tensors[0][0]), QNN_TENSOR_GET_RANK(output_tensors[0][0]));
    datautil::StatusCode datautilStatus{datautil::StatusCode::SUCCESS};
    std::tie(datautilStatus, length) = datautil::calculateLength(dims, QNN_TENSOR_GET_DATA_TYPE(output_tensors[0][0]));

    //Reserve space for the buffer that will keep the unconditional text embedding
    uncondBuffer_text_enc = QNN_CLIENT_BUFFER_INIT;
    m_ioTensor.allocateBuffer(reinterpret_cast<uint8_t**>(&uncondBuffer_text_enc.data),
                                  dims,
                                  QNN_TENSOR_GET_DATA_TYPE(output_tensors[0][0]));
    uncondBuffer_text_enc.dataSize = length;
    
    //Reserve space for the buffer that will keep the conditional text embedding
    condBuffer_text_enc = QNN_CLIENT_BUFFER_INIT;
    m_ioTensor.allocateBuffer(reinterpret_cast<uint8_t**>(&condBuffer_text_enc.data),
                                  dims,
                                  QNN_TENSOR_GET_DATA_TYPE(output_tensors[0][0]));
    condBuffer_text_enc.dataSize = length;

    //Make the inference to get the unconditional text embedding and copy the output in the corresponding backup buffer
    //The empty string text embedding is done during the setupTensors process
    m_qnnFunctionPointers.qnnInterface.graphExecute(graphInfo.graph, input_tensors[0], graphInfo.numInputTensors, output_tensors[0], graphInfo.numOutputTensors, m_profileBackendHandle, nullptr);
    uint8_t *uncond_buff = reinterpret_cast<uint8_t*>(uncondBuffer_text_enc.data);
    uint8_t *buff = reinterpret_cast<uint8_t*>(QNN_TENSOR_GET_CLIENT_BUF(output_tensors[0][0]).data);
    for(size_t i=0; i<length; i++){
      *(uncond_buff + i) = *(buff + i);
    }

    //returnStatus = copyFromFloatToNative(tokensId.data(), input);
    int8_t * pointerToBuffer = static_cast<int8_t*>(QNN_TENSOR_GET_CLIENT_BUF(input_tensors[0][0]).data);
    int8_t * pointertoInputData = reinterpret_cast<int8_t*>(tokensId.data());
    for(size_t i=0; i<QNN_TENSOR_GET_CLIENT_BUF(input_tensors[0][0]).dataSize; i++){
      pointerToBuffer[i] = pointertoInputData[i];
    }

    m_qnnFunctionPointers.qnnInterface.graphExecute(graphInfo.graph, input_tensors[0], graphInfo.numInputTensors, output_tensors[0], graphInfo.numOutputTensors, m_profileBackendHandle, nullptr);
    uint8_t *cond_buff = reinterpret_cast<uint8_t*>(condBuffer_text_enc.data);
    buff = reinterpret_cast<uint8_t*>(QNN_TENSOR_GET_CLIENT_BUF(output_tensors[0][0]).data);
    for(size_t i=0; i<length; i++){
      *(cond_buff + i) = *(buff + i);
    }

}

void speech_to_image::QnnSpeechToImage::executeLoop(){
  //Select the graph corresponding to the unet model for execution
  auto graphInfo = (*m_graphsInfo)[1];

  //Get the information of the output tensor (Required to configure the two buffers that will be used to keep the two outputs)
  size_t length{0};
  std::vector<size_t> dims;
  m_ioTensor.fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(output_tensors[1][0]), QNN_TENSOR_GET_RANK(output_tensors[1][0]));
  datautil::StatusCode datautilStatus{datautil::StatusCode::SUCCESS};
  std::tie(datautilStatus, length) = datautil::calculateLength(dims, QNN_TENSOR_GET_DATA_TYPE(output_tensors[1][0]));

  //Reserve space for the buffer that will keep the unconditional noise prediction
  uncondBuffer_noise_predict = QNN_CLIENT_BUFFER_INIT;
  m_ioTensor.allocateBuffer(reinterpret_cast<uint8_t**>(&uncondBuffer_noise_predict.data),
                                  dims,
                                  QNN_TENSOR_GET_DATA_TYPE(output_tensors[1][0]));
  uncondBuffer_noise_predict.dataSize = length;

  uint8_t *unet_buff_out_ptr = reinterpret_cast<uint8_t*>(QNN_TENSOR_GET_CLIENT_BUF(output_tensors[1][0]).data);
  uint8_t *uncond_buff_noisePredict_ptr = reinterpret_cast<uint8_t*>(uncondBuffer_noise_predict.data);
  
  std::vector<float> output;
  std::vector<float> copy_output(16384);

  for(size_t step=0; step<20; ++step){
    if(stop_stable_diffusion){
      break;
    }

    //The first input of the Unet model, which corresponds to the seed/scheduler-output was populated during the tensor initialization
    //Copy the unconditional text embedded in the output of the text_encoder
    uint8_t *uncond_buff = reinterpret_cast<uint8_t*>(uncondBuffer_text_enc.data);
    uint8_t *buff = reinterpret_cast<uint8_t*>(QNN_TENSOR_GET_CLIENT_BUF(output_tensors[0][0]).data);
    for(size_t i=0; i<length; i++){
      *(buff + i) = *(uncond_buff + i);
    }

    //Copy the output of the text encoder (the unconditioned one) in the input of the unet model    
    float *tmp_buff = nullptr;
    m_ioTensor.convertToFloat(&tmp_buff, &(output_tensors[0][0]));
    m_ioTensor.copyFromFloatToNative(tmp_buff, &(input_tensors[1][2]));
    
    //Generate the embeddings of the timeStep and put it in the corresponding input tensor
    float timesteps_embeded[1280];
    datautil::get_timestep_embedding(step, 1280, timesteps_embeded);
    m_ioTensor.copyFromFloatToNative(timesteps_embeded, &(input_tensors[1][1]));

    //Make the inference to get the unconditional noise prediction and copy the result in the corresponding buffer
    m_qnnFunctionPointers.qnnInterface.graphExecute(graphInfo.graph, input_tensors[1], graphInfo.numInputTensors, output_tensors[1], graphInfo.numOutputTensors, m_profileBackendHandle, nullptr);
    for(size_t i=0; i<length; i++){
      *(uncond_buff_noisePredict_ptr + i) = *(unet_buff_out_ptr + i);
    }

    //Copy the unconditional text embedded in the output of the text_encoder
    uint8_t *cond_buff = reinterpret_cast<uint8_t*>(condBuffer_text_enc.data);
    buff = reinterpret_cast<uint8_t*>(QNN_TENSOR_GET_CLIENT_BUF(output_tensors[0][0]).data);
    for(size_t i=0; i<length; i++){
      *(buff + i) = *(cond_buff + i);
    }

    //Copy the output of the text encoder (the conditioned one) in the input of the unet model
    tmp_buff = nullptr;
    m_ioTensor.convertToFloat(&tmp_buff, &(output_tensors[0][0]));
    m_ioTensor.copyFromFloatToNative(tmp_buff, &(input_tensors[1][2]));
    //Make the inference to get the conditional noise prediction and copy the result in the corresponding buffer
    m_qnnFunctionPointers.qnnInterface.graphExecute(graphInfo.graph, input_tensors[1], graphInfo.numInputTensors, output_tensors[1], graphInfo.numOutputTensors, m_profileBackendHandle, nullptr);

    //TODO: Invoke the scheduller reinterpreting the buffers as the corresponding output data type
    output = datautil::run_scheduler(output_tensors[1][0],
                            uncondBuffer_noise_predict,
                            input_tensors[1][0],
                            step);
    

    m_ioTensor.copyFromFloatToNative(output.data(), &(input_tensors[1][0]));

  }

  //Select the graph corresponding to the vae_decoder model for execution
  graphInfo = (*m_graphsInfo)[2];

  m_ioTensor.copyFromFloatToNative(output.data(), &(input_tensors[2][0]));

  float *tmp_buff = nullptr;
  m_ioTensor.convertToFloat(&tmp_buff, &(input_tensors[2][0]));


  m_qnnFunctionPointers.qnnInterface.graphExecute(graphInfo.graph, input_tensors[2], graphInfo.numInputTensors, output_tensors[2], graphInfo.numOutputTensors, m_profileBackendHandle, nullptr);

  tmp_buff = nullptr;
  m_ioTensor.convertToFloat(&tmp_buff, &(output_tensors[2][0]));
  //m_ioTensor.convertAndWriteOutputTensorInFloat(&(output_tensors[2][0]), {assets_path}, "output.raw");

  raw_image_to_png(tmp_buff, (assets_path + name_png).c_str());
}

//runWhisperEncoder
void speech_to_image::QnnSpeechToImage::runWhisperEncoder(){
    auto graphInfo = (*m_graphsInfo)[models::ENCODER];
    QNN_ERROR("runWhisperEncoder");
    m_qnnFunctionPointers.qnnInterface.graphExecute(graphInfo.graph, input_tensors[models::ENCODER], graphInfo.numInputTensors, output_tensors[models::ENCODER], graphInfo.numOutputTensors, m_profileBackendHandle, nullptr);

    //copying k-cache encoder output in k-cache-cross decoder input
    float* output_tensor_encoder = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::ENCODER][encoder_output::K_CACHE_CROSS_0]).data);
    float* input_tensor_decoder  = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(input_tensors[models::DECODER][decoder_input::K_CACHE_CROSS_0]).data);
    for(size_t i=0; i<QNN_TENSOR_GET_CLIENT_BUF(input_tensors[models::DECODER][decoder_input::K_CACHE_CROSS_0]).dataSize / 4; i++){
        input_tensor_decoder[i] = output_tensor_encoder[i];
    }

    output_tensor_encoder = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::ENCODER][encoder_output::K_CACHE_CROSS_1]).data);
    input_tensor_decoder  = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(input_tensors[models::DECODER][decoder_input::K_CACHE_CROSS_1]).data);
    for(size_t i=0; i<QNN_TENSOR_GET_CLIENT_BUF(input_tensors[models::DECODER][decoder_input::K_CACHE_CROSS_1]).dataSize / 4; i++){
        input_tensor_decoder[i] = output_tensor_encoder[i];
    }

    output_tensor_encoder = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::ENCODER][encoder_output::K_CACHE_CROSS_2]).data);
    input_tensor_decoder  = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(input_tensors[models::DECODER][decoder_input::K_CACHE_CROSS_2]).data);
    for(size_t i=0; i<QNN_TENSOR_GET_CLIENT_BUF(input_tensors[models::DECODER][decoder_input::K_CACHE_CROSS_2]).dataSize / 4; i++){
        input_tensor_decoder[i] = output_tensor_encoder[i];
    }

    output_tensor_encoder = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::ENCODER][encoder_output::K_CACHE_CROSS_3]).data);
    input_tensor_decoder  = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(input_tensors[models::DECODER][decoder_input::K_CACHE_CROSS_3]).data);
    for(size_t i=0; i<QNN_TENSOR_GET_CLIENT_BUF(input_tensors[models::DECODER][decoder_input::K_CACHE_CROSS_3]).dataSize / 4; i++){
        input_tensor_decoder[i] = output_tensor_encoder[i];
    }

    //copying v-cache encoder output in v-cache-cross decoder input
    output_tensor_encoder = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::ENCODER][encoder_output::V_CACHE_CROSS_0]).data);
    input_tensor_decoder = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(input_tensors[models::DECODER][decoder_input::V_CACHE_CROSS_0]).data);
    for(size_t i=0; i<QNN_TENSOR_GET_CLIENT_BUF(input_tensors[models::DECODER][decoder_input::V_CACHE_CROSS_0]).dataSize / 4; i++){
        input_tensor_decoder[i] = output_tensor_encoder[i];
    }

    output_tensor_encoder = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::ENCODER][encoder_output::V_CACHE_CROSS_1]).data);
    input_tensor_decoder = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(input_tensors[models::DECODER][decoder_input::V_CACHE_CROSS_1]).data);
    for(size_t i=0; i<QNN_TENSOR_GET_CLIENT_BUF(input_tensors[models::DECODER][decoder_input::V_CACHE_CROSS_1]).dataSize / 4; i++){
        input_tensor_decoder[i] = output_tensor_encoder[i];
    }

    output_tensor_encoder = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::ENCODER][encoder_output::V_CACHE_CROSS_2]).data);
    input_tensor_decoder = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(input_tensors[models::DECODER][decoder_input::V_CACHE_CROSS_2]).data);
    for(size_t i=0; i<QNN_TENSOR_GET_CLIENT_BUF(input_tensors[models::DECODER][decoder_input::V_CACHE_CROSS_2]).dataSize / 4; i++){
        input_tensor_decoder[i] = output_tensor_encoder[i];
    }

    output_tensor_encoder = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::ENCODER][encoder_output::V_CACHE_CROSS_3]).data);
    input_tensor_decoder = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(input_tensors[models::DECODER][decoder_input::V_CACHE_CROSS_3]).data);
    for(size_t i=0; i<QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::ENCODER][encoder_output::V_CACHE_CROSS_3]).dataSize / 4; i++){
        input_tensor_decoder[i] = output_tensor_encoder[i];
    }
}


namespace qnn_utils {
// Count elements given byte size and element size
    inline size_t num_elems(size_t bytes, size_t elem_size) {
        return (elem_size == 0) ? 0 : (bytes / elem_size);
    }

// Write a scalar int32 into a buffer (POSITION_IDS or INPUT_IDS)
    inline void write_scalar_int32(void* data, size_t bytes, int32_t value) {
        if (num_elems(bytes, sizeof(int32_t)) >= 1) {
            *reinterpret_cast<int32_t*>(data) = value;
        }
    }

// Fill a float buffer with a constant (attention mask init)
    inline void fill_float32(void* data, size_t bytes, float value) {
        const size_t N = num_elems(bytes, sizeof(float));
        auto* p = reinterpret_cast<float*>(data);
        for (size_t i = 0; i < N; ++i) p[i] = value;
    }

// Allow a single column (index) in attention mask by writing 0.0 there.
// Assumes flat layout [1,1,1, MEAN_DECODE_LEN] → contiguous float array length MEAN_DECODE_LEN
    inline void allow_attention_column(void* data, size_t bytes, int column) {
        if (column < 0) return;
        const size_t N = num_elems(bytes, sizeof(float));
        if (static_cast<size_t>(column) < N) {
            reinterpret_cast<float*>(data)[column] = 0.0f;
        }
    }

// Zero out a buffer
    inline void zero_mem(void* dst, size_t bytes) {
        std::memset(dst, 0, bytes);
    }

// Copy bytes (size-safe: copy min(dstBytes, srcBytes))
    inline void copy_bytes(void* dst, size_t dstBytes, const void* src, size_t srcBytes) {
        const size_t n = std::min(dstBytes, srcBytes);
        std::memcpy(dst, src, n);
    }

} // namespace qnn_utils


std::string speech_to_image::QnnSpeechToImage::runWhisperDecoder() {
    auto graphInfo = (*m_graphsInfo)[models::DECODER];
    QNN_ERROR("runWhisperDecoder");

    // ---------------- Configuration ----------------
    const float   MASK_NEG           = -100.0f;
    const int     MEAN_DECODE_LEN    = 200;
    const int     NUM_DECODER_LAYERS = 4;

    // ---------------- Convenience: raw pointers & sizes to IO tensors ----------------
    // Inputs
    auto& in_input_ids_buf     = input_tensors [models::DECODER][decoder_input ::INPUT_IDS];
    auto& in_position_ids_buf  = input_tensors [models::DECODER][decoder_input ::POSITION_IDS];

    void*  in_input_ids_ptr     = QNN_TENSOR_GET_CLIENT_BUF(in_input_ids_buf).data;
    size_t in_input_ids_bytes   = QNN_TENSOR_GET_CLIENT_BUF(in_input_ids_buf).dataSize;

    void*  in_position_ids_ptr  = QNN_TENSOR_GET_CLIENT_BUF(in_position_ids_buf).data;
    size_t in_position_ids_bytes= QNN_TENSOR_GET_CLIENT_BUF(in_position_ids_buf).dataSize;

    // If your model has ATTENTION_MASK as input, uncomment this block.
    auto& in_attention_buf      = input_tensors [models::DECODER][decoder_input ::ATTENTION_MASK];
    void*  in_attention_ptr     = QNN_TENSOR_GET_CLIENT_BUF(in_attention_buf).data;
    size_t in_attention_bytes   = QNN_TENSOR_GET_CLIENT_BUF(in_attention_buf).dataSize;
    const bool hasAttention     = true;

    // Self KV caches (IN/OUT)
    // K self IN
    void*  k_self_in_ptr [NUM_DECODER_LAYERS];
    size_t k_self_in_bytes[NUM_DECODER_LAYERS];
    k_self_in_ptr [0] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::K_CACHE_SELF_0]).data;
    k_self_in_bytes[0] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::K_CACHE_SELF_0]).dataSize;
    k_self_in_ptr [1] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::K_CACHE_SELF_1]).data;
    k_self_in_bytes[1] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::K_CACHE_SELF_1]).dataSize;
    k_self_in_ptr [2] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::K_CACHE_SELF_2]).data;
    k_self_in_bytes[2] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::K_CACHE_SELF_2]).dataSize;
    k_self_in_ptr [3] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::K_CACHE_SELF_3]).data;
    k_self_in_bytes[3] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::K_CACHE_SELF_3]).dataSize;

    // V self IN
    void*  v_self_in_ptr [NUM_DECODER_LAYERS];
    size_t v_self_in_bytes[NUM_DECODER_LAYERS];
    v_self_in_ptr [0] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::V_CACHE_SELF_0]).data;
    v_self_in_bytes[0] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::V_CACHE_SELF_0]).dataSize;
    v_self_in_ptr [1] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::V_CACHE_SELF_1]).data;
    v_self_in_bytes[1] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::V_CACHE_SELF_1]).dataSize;
    v_self_in_ptr [2] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::V_CACHE_SELF_2]).data;
    v_self_in_bytes[2] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::V_CACHE_SELF_2]).dataSize;
    v_self_in_ptr [3] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::V_CACHE_SELF_3]).data;
    v_self_in_bytes[3] = QNN_TENSOR_GET_CLIENT_BUF(input_tensors [models::DECODER][decoder_input ::V_CACHE_SELF_3]).dataSize;

    // Outputs
    auto& out_logits_buf   = output_tensors[models::DECODER][decoder_output::LOGITS];
    const void* out_logits_ptr   = QNN_TENSOR_GET_CLIENT_BUF(out_logits_buf).data;
    size_t      out_logits_bytes = QNN_TENSOR_GET_CLIENT_BUF(out_logits_buf).dataSize;

    // K self OUT
    const void* k_self_out_ptr [NUM_DECODER_LAYERS];
    size_t      k_self_out_bytes[NUM_DECODER_LAYERS];
    k_self_out_ptr [0] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::K_CACHE_SELF_0]).data;
    k_self_out_bytes[0] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::K_CACHE_SELF_0]).dataSize;
    k_self_out_ptr [1] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::K_CACHE_SELF_1]).data;
    k_self_out_bytes[1] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::K_CACHE_SELF_1]).dataSize;
    k_self_out_ptr [2] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::K_CACHE_SELF_2]).data;
    k_self_out_bytes[2] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::K_CACHE_SELF_2]).dataSize;
    k_self_out_ptr [3] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::K_CACHE_SELF_3]).data;
    k_self_out_bytes[3] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::K_CACHE_SELF_3]).dataSize;

    // V self OUT
    const void* v_self_out_ptr [NUM_DECODER_LAYERS];
    size_t      v_self_out_bytes[NUM_DECODER_LAYERS];
    v_self_out_ptr [0] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::V_CACHE_SELF_0]).data;
    v_self_out_bytes[0] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::V_CACHE_SELF_0]).dataSize;
    v_self_out_ptr [1] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::V_CACHE_SELF_1]).data;
    v_self_out_bytes[1] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::V_CACHE_SELF_1]).dataSize;
    v_self_out_ptr [2] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::V_CACHE_SELF_2]).data;
    v_self_out_bytes[2] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::V_CACHE_SELF_2]).dataSize;
    v_self_out_ptr [3] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::V_CACHE_SELF_3]).data;
    v_self_out_bytes[3] = QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::V_CACHE_SELF_3]).dataSize;

    // ---------------- Initial tokens & state ----------------
    {
        auto* input_ids_scalar = reinterpret_cast<int32_t*>(in_input_ids_ptr);
    }

    // position_ids = 0
    qnn_utils::write_scalar_int32(in_position_ids_ptr, in_position_ids_bytes, 0);

    // Initialize attention mask: all NEG, reveal last column for n=0
    if (hasAttention) {
        qnn_utils::fill_float32(in_attention_ptr, in_attention_bytes, MASK_NEG);
        // column = MEAN_DECODE_LEN - 0 - 1
        qnn_utils::allow_attention_column(in_attention_ptr, in_attention_bytes, MEAN_DECODE_LEN - 1);
    }

    // Zero‑init self‑attention caches on first step
    for (int L = 0; L < NUM_DECODER_LAYERS; ++L) {
        qnn_utils::zero_mem(k_self_in_ptr[L], k_self_in_bytes[L]);
        qnn_utils::zero_mem(v_self_in_ptr[L], v_self_in_bytes[L]);
    }

    std::string result;

    // ---------------- Autoregressive decode loop ----------------
    for (size_t n = 0; n < MAX_ITERATIONS; ++n) {
        // Execute decoder
        m_qnnFunctionPointers.qnnInterface.graphExecute(
                graphInfo.graph,
                input_tensors [models::DECODER], graphInfo.numInputTensors,
                output_tensors[models::DECODER], graphInfo.numOutputTensors,
                m_profileBackendHandle, nullptr);

        // Re-read output pointers (some runtimes rewrite client buffers)
        out_logits_ptr   = QNN_TENSOR_GET_CLIENT_BUF(out_logits_buf).data;
        out_logits_bytes = QNN_TENSOR_GET_CLIENT_BUF(out_logits_buf).dataSize;

        // Calculating max logits index to get the token with higher probability
        float* pointerToBuffer = reinterpret_cast<float*>(QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::LOGITS]).data);
        float max = std::numeric_limits<float>::lowest();
        size_t max_index = 0;

        for(size_t i = 0; i < QNN_TENSOR_GET_CLIENT_BUF(output_tensors[models::DECODER][decoder_output::LOGITS]).dataSize / 4; i++){
            if(pointerToBuffer[i] > max){
                max = pointerToBuffer[i];
                max_index = i;
            }
        }

        result += std::to_string(max_index);
        // EOS → stop
        if (static_cast<int32_t>(max_index) == END_TOKEN) {
            break;
        } else {
            result += ",";
        }

        // Prepare inputs for next iteration

        // 1) POSITION_IDS += 1
        {
            auto* pos_ptr = reinterpret_cast<int32_t*>(in_position_ids_ptr);
            if (qnn_utils::num_elems(in_position_ids_bytes, sizeof(int32_t)) >= 1) {
                *pos_ptr = *pos_ptr + 1;
            }
        }

        // 2) INPUT_IDS = max_index (token we just emitted)
        qnn_utils::write_scalar_int32(in_input_ids_ptr, in_input_ids_bytes, static_cast<int32_t>(max_index));
        {
            auto* input_ids_scalar = reinterpret_cast<int32_t*>(in_input_ids_ptr);
        }

        // 3) Self KV cache hand‑off: *_OUT → *_IN for every layer
        for (int L = 0; L < NUM_DECODER_LAYERS; ++L) {
            qnn_utils::copy_bytes(k_self_in_ptr[L], k_self_in_bytes[L], k_self_out_ptr[L], k_self_out_bytes[L]);
            qnn_utils::copy_bytes(v_self_in_ptr[L], v_self_in_bytes[L], v_self_out_ptr[L], v_self_out_bytes[L]);
        }

        // 4) Attention mask progression (if present)
        if (hasAttention) {
            // Reveal column: MEAN_DECODE_LEN - (n+1) - 1
            const int nextCol = MEAN_DECODE_LEN - static_cast<int>(n+1) - 1;
            qnn_utils::allow_attention_column(in_attention_ptr, in_attention_bytes, nextCol);
        }
    }

    return result;
}