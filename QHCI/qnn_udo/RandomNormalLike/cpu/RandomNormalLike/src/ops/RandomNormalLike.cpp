//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

//==============================================================================
// Auto Generated Code for RandomNormalLike
//==============================================================================
#include <iostream>
#include <string>

#include "CpuBackendUtils.hpp"
#include "CustomOpPackage.hpp"
#include "math.h"

const float mean = 0;
const float scale = 1;

float randn(float mu, float sigma) {
  float U1, U2, W, mult;
  static float X1, X2;
  static int call = 0;
  if (call == 1) {
    call = !call;
    return (mu + sigma * (float) X2);
  }

  do {
    U1 = -1 + ((float) rand () / (float)RAND_MAX) * 2;
    U2 = -1 + ((float) rand () / (float)RAND_MAX) * 2;
    W = pow (U1, 2) + pow (U2, 2);
  }
  while (W >= 1 || W == 0); 

  mult = sqrt ((-2 * log (W)) / W);
  X1 = U1 * mult;
  X2 = U2 * mult;

  call = !call;

  return (mu + sigma * (float) X1);
}

using namespace qnn::custom;
using namespace qnn::custom::utils;

namespace randomnormallike {

Qnn_ErrorHandle_t execute(CustomOp* operation) {

  /**
   * Add code here
   **/

  auto input           = operation->getInput(0);
  auto output          = operation->getOutput(0);
  uint32_t* input_dims = input->currentDimensions;
  int B                = *(input_dims + 0);
  int H                = *(input_dims + 1);
  int W                = *(input_dims + 2);
  int C                = *(input_dims + 3);
  float* in0           = reinterpret_cast<float*>(input->data);
  float* result        = reinterpret_cast<float*>(output->data);

  for(int b_it = 0 ; b_it < B; b_it++)
  for(int h_it = 0 ; h_it < H; h_it++)
  for(int w_it = 0 ; w_it < W; w_it++ )
  for(int d_it = 0 ; d_it < C; d_it++ ) {
      result[b_it * H * W * C + h_it * W * C + w_it * C + d_it] = randn(mean, scale);
      //printf("result[%d, %d, %d, %d,] = %f\n", b_it, h_it, w_it, d_it, result[b_it * H * W * C + h_it * W * C + w_it * C + d_it]);
  }

  return QNN_SUCCESS;
}

Qnn_ErrorHandle_t finalize(const CustomOp* operation) {
  QNN_CUSTOM_BE_ENSURE_EQ(operation->numInput(), 1, QNN_OP_PACKAGE_ERROR_VALIDATION_FAILURE)
  QNN_CUSTOM_BE_ENSURE_EQ(operation->numOutput(), 1, QNN_OP_PACKAGE_ERROR_VALIDATION_FAILURE)

  /**
   * Add code here
   **/

  return QNN_SUCCESS;
}

Qnn_ErrorHandle_t free(CustomOp& operation) {

    /**
    * Add code here
    **/

    return QNN_SUCCESS;
}

Qnn_ErrorHandle_t populateFromNode(const QnnOpPackage_Node_t node,
                                   QnnOpPackage_GraphInfrastructure_t graphInfrastructure,
                                   CustomOp* operation) {
  // Add input
  for (uint32_t i = 0; i < numInputs(node); i++) {
    operation->addInput(getInput(node, i));
  }

  // Add output
  for (uint32_t i = 0; i < numOutputs(node); i++) {
    operation->addOutput(getOutput(node, i));
  }


  return QNN_SUCCESS;
}

Qnn_ErrorHandle_t validateOpConfig(Qnn_OpConfig_t opConfig) {
  QNN_CUSTOM_BE_ENSURE_EQ(
      strcmp(opConfig.v1.typeName, "RandomNormalLike"), 0, QNN_OP_PACKAGE_ERROR_INVALID_ARGUMENT)

  QNN_CUSTOM_BE_ENSURE_EQ(opConfig.v1.numOfInputs, 1, QNN_OP_PACKAGE_ERROR_VALIDATION_FAILURE)
  QNN_CUSTOM_BE_ENSURE_EQ(opConfig.v1.numOfOutputs, 1, QNN_OP_PACKAGE_ERROR_VALIDATION_FAILURE)

  return QNN_SUCCESS;
}
}  // namespace randomnormallike

CustomOpRegistration_t* register_RandomnormallikeCustomOp() {
  using namespace randomnormallike;
  static CustomOpRegistration_t RandomnormallikeRegister = {execute, finalize, free, validateOpConfig, populateFromNode};
  return &RandomnormallikeRegister;
}

REGISTER_OP(RandomNormalLike, register_RandomnormallikeCustomOp);
