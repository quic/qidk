//==============================================================================
//
// Copyright (c) 2020-2023 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

#pragma once
#include <stddef.h>
#include <stdint.h>

#include <string>
#include <utility>

#include "QnnOpPackage.h"
#include "QnnTypes.h"

//============================================================================
// Backend Defined Behavior
//=============================================================================
// A required backend defined tensor object which designates an input or output tensor
typedef struct CustomOpTensor* CustomOpTensorPtr_t;

// A required backend defined parameter object which designates scalar, tensor and string parameters
typedef struct CustomOpParam* CustomOpParamPtr_t;

// A backend defined object which contains additional info about an operation such as connectivity,
// buffers etc
typedef struct CustomOpContext* CustomOpContextPtr_t;

// A backend defined object which contains information about a kernel such as its string path, its
// buffers, assigned memory, local dimensions etc.
typedef struct CustomOpKernelContext* CustomOpKernelContextPtr_t;

namespace qnn {

namespace custom {

namespace utils {

// Each backend is expected to define these utilities to aid users in accessing basic info about
// an operation package node.
const CustomOpTensorPtr_t* getInput(QnnOpPackage_Node_t node);

const CustomOpTensorPtr_t* getOutput(QnnOpPackage_Node_t node);

const CustomOpParamPtr_t* getParam(QnnOpPackage_Node_t node);

const CustomOpTensorPtr_t getInput(QnnOpPackage_Node_t node, size_t idx);

CustomOpTensorPtr_t getOutput(QnnOpPackage_Node_t node, size_t idx);

const std::pair<bool, CustomOpParamPtr_t> getParam(QnnOpPackage_Node_t node,
                                                   const std::string& paramName);

uint32_t numInputs(QnnOpPackage_Node_t node);

uint32_t numOutputs(QnnOpPackage_Node_t node);

uint32_t numDimensions(CustomOpTensorPtr_t tensor);

const uint32_t* getTensorShape(CustomOpTensorPtr_t tensor);

void* getTensorData(CustomOpTensorPtr_t tensor);

uint32_t numTensorSize(CustomOpTensorPtr_t tensor);
// Additional backend utilities should be included under this namespace
namespace backend_utils {}
}  // namespace utils
}  // namespace custom
}  // namespace qnn
