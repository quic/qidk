//==============================================================================
//
// Copyright (c) 2020, 2023 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

#include <string.h>

#include <string>

#include "CpuBackendUtils.hpp"

namespace qnn {

namespace custom {

namespace utils {

// Each backend is expected to define these utilities to aid users in accessing basic info about
// an operation package node.
const CustomOpTensorPtr_t* getInput(QnnOpPackage_Node_t node) {
  return (CustomOpTensorPtr_t*)reinterpret_cast<QnnCpuOpPackage_Node_t*>(node)->inputs;
}

const CustomOpTensorPtr_t* getOutput(QnnOpPackage_Node_t node) {
  return (CustomOpTensorPtr_t*)reinterpret_cast<QnnCpuOpPackage_Node_t*>(node)->outputs;
}

const CustomOpParamPtr_t* getParam(QnnOpPackage_Node_t node) {
  return (CustomOpParamPtr_t*)reinterpret_cast<QnnCpuOpPackage_Node_t*>(node)->params;
}

const std::pair<bool, CustomOpParamPtr_t> getParam(QnnOpPackage_Node_t node,
                                                   const std::string& name) {
  auto cpuNode = reinterpret_cast<QnnCpuOpPackage_Node_t*>(node);
  auto params  = (CustomOpParamPtr_t*)cpuNode->params;

  for (uint32_t idx = 0; idx < cpuNode->numOfParams; idx++) {
    auto paramName = params[idx]->name;

    if (strcmp(paramName, name.c_str()) == 0) {
      return {true, params[idx]};
    }
  }

  return {false, nullptr};
}

const CustomOpTensorPtr_t getInput(QnnOpPackage_Node_t node, size_t idx) {
  return (CustomOpTensorPtr_t) reinterpret_cast<QnnCpuOpPackage_Node_t*>(node)->inputs[idx];
}

CustomOpTensorPtr_t getOutput(QnnOpPackage_Node_t node, size_t idx) {
  return (CustomOpTensorPtr_t) reinterpret_cast<QnnCpuOpPackage_Node_t*>(node)->outputs[idx];
}

uint32_t numInputs(QnnOpPackage_Node_t node) {
  return reinterpret_cast<QnnCpuOpPackage_Node_t*>(node)->numOfInputs;
}

uint32_t numOutputs(QnnOpPackage_Node_t node) {
  return reinterpret_cast<QnnCpuOpPackage_Node_t*>(node)->numOfOutputs;
}

uint32_t numDimensions(CustomOpTensorPtr_t tensor) {
  return reinterpret_cast<QnnCpuOpPackage_Tensor_t*>(tensor)->rank;
}

uint32_t numTensorSize(CustomOpTensorPtr_t tensor) {
  uint32_t size  = 1;
  auto cpuTensor = reinterpret_cast<QnnCpuOpPackage_Tensor_t*>(tensor);

  for (uint32_t i = 0; i < cpuTensor->rank; i++) {
    size *= cpuTensor->currentDimensions[i];
  }
  return size;
}

const uint32_t* getTensorShape(CustomOpTensorPtr_t tensor) {
  return reinterpret_cast<QnnCpuOpPackage_Tensor_t*>(tensor)->currentDimensions;
}

template <typename T>
const T* getTensorData(CustomOpTensorPtr_t tensor) {
  auto tempTensor = reinterpret_cast<QnnCpuOpPackage_Tensor_t*>(tensor);
  auto dataRef    = reinterpret_cast<T*>(tempTensor->data);
  return const_cast<T*>(dataRef);
}

template <typename T>
T& getTensorDataRef(CustomOpTensorPtr_t tensor) {
  auto tempTensor = reinterpret_cast<QnnCpuOpPackage_Tensor_t*>(tensor);
  auto dataRef    = reinterpret_cast<T*>(tempTensor->data);
  return &dataRef;
}

namespace backend_utils {

const double getScalarParam(const CustomOpParamPtr_t param) {
  auto cpuParam = reinterpret_cast<QnnCpuOpPackage_Param_t*>(param);
  return cpuParam->scalarParam;
}

const CustomOpTensorPtr_t getTensorParam(const CustomOpParamPtr_t param) {
  auto cpuParam = reinterpret_cast<QnnCpuOpPackage_Param_t*>(param);
  return (CustomOpTensorPtr_t)cpuParam->tensorParam;
}

}  // namespace backend_utils
}  // namespace utils
}  // namespace custom
}  // namespace qnn
