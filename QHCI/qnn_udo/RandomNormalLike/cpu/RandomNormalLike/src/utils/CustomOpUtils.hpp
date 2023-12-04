//==============================================================================
//
// Copyright (c) 2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "BackendUtils.hpp"
#include "CustomBEMacros.hpp"

namespace qnn {

namespace custom {

namespace utils {

/**
 * @brief Helper class that can hold information that holds information extracted
 * from a QNN node. This class function signatures and private members are freely extensible and
 * modifiable. The public class member variables must remain unchanged.
 */

class CustomOp {
 public:
  const char* m_name;
  const char* m_typeName;
  uint32_t m_numKernels;

  CustomOp()          = default;
  virtual ~CustomOp() = default;

  /**
   * @brief The custom op constructor
   * @param name The name of the operation
   * @param typeName The type of the operation
   * @return
   */
  CustomOp(const char* name, const char* typeName) : m_name(name), m_typeName(typeName) {}

  /**
   * @brief Adds an input tensor to the operation
   * @param inTensor An input tensor to this operation as defined by each backend
   * @return
   */
  virtual Qnn_ErrorHandle_t addInput(CustomOpTensorPtr_t inTensor) {
    m_Inputs.emplace_back(inTensor);
    return QNN_SUCCESS;
  };

  /**
   * @brief Adds an output tensor to the operation
   * @param outTensor An output tensor of this operation as defined by each backend
   * @return
   */
  virtual Qnn_ErrorHandle_t addOutput(CustomOpTensorPtr_t outTensor) {
    m_Outputs.emplace_back(outTensor);
    return QNN_SUCCESS;
  };

  /**
   * Adds the parameter name
   * @param paramName The name of each parameter to be added
   * @param param The param object to be added as defined by the backend
   * @return
   */
  virtual Qnn_ErrorHandle_t addParam(const std::string& paramName, CustomOpParamPtr_t param) {
    m_Params[paramName] = param;
    return QNN_SUCCESS;
  };

  /**
   * Returns a pointer to the input tensor specified by index
   * @param index
   * @return
   */
  const CustomOpTensorPtr_t getInput(size_t index = 0) const { return m_Inputs[index]; }

  /**
   * Returns a reference to the output tensor specified by index
   * @param index
   * @return
   */
  CustomOpTensorPtr_t getOutput(size_t index = 0) const { return m_Outputs[index]; }

  /**
   * Returns a reference to the output tensor data
   * @param index
   * @return
   */
  CustomOpTensorPtr_t* getOutputsFlat() { return m_Outputs.data(); }

  /** Returns the requested parameter specified by name
   * @param name the name of the parameter
   */
  CustomOpParamPtr_t getParam(const std::string& name) { return m_Params[name]; }

  /**
   *
   * @return The number of inputs
   */
  uint32_t numInput() const { return m_Inputs.size(); }

  /**
   *
   * @return The number of outputs
   */
  uint32_t numOutput() const { return m_Outputs.size(); }

 protected:
  std::vector<CustomOpTensorPtr_t> m_Inputs;
  std::vector<CustomOpTensorPtr_t> m_Outputs;
  std::map<std::string, CustomOpParamPtr_t> m_Params;
  std::unique_ptr<CustomOpTensorPtr_t> m_tempTensor;
};

}  // namespace utils
}  // namespace custom
}  // namespace qnn
