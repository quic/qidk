//==============================================================================
//
// Copyright (c) 2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

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