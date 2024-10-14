//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#pragma once

#include "BackendUtils.hpp"
#include "QnnCpuOpPackage.h"

// Tensor and parameter definitions
struct CustomOpTensor : public QnnCpuOpPackage_Tensor_t {};

struct CustomOpParam : public QnnCpuOpPackage_Param_t {};

namespace qnn {
namespace custom {
namespace utils {
namespace backend_utils {

const double getScalarParam(const CustomOpParamPtr_t param);

const CustomOpTensorPtr_t getTensorParam(const CustomOpParamPtr_t param);
}  // namespace backend_utils
}  // namespace utils
}  // namespace custom
}  // namespace qnn
