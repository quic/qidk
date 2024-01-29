// -*- mode: cpp -*-
// =============================================================================
// @@-COPYRIGHT-START-@@
//
// Copyright (c) 2023 of Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
//
// @@-COPYRIGHT-END-@@
// =============================================================================
#ifndef SNPEHELPER_H
#define SNPEHELPER_H

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <sys/stat.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "Utils.h"
#include "SNPERuntime.h"
#include <pybind11/numpy.h>

using namespace std;
namespace py = pybind11;

class SnpeContext
{
public:
    std::unique_ptr<snperuntime::SNPERuntime> m_snperuntime;
    std::vector<std::string> m_inputLayers;
    std::vector<std::string> m_outputLayers;
    std::vector<std::string> m_outputTensors;
    std::map<std::string, int> map_m_outputTensors, map_m_inputLayers;
    std::string m_model_type;
    std::string m_model_path;
    runtime_t m_runtime;
    performance_t m_profiling_level;
    bool m_enable_init_cache;
    float *input_tensor, *output_tensor;
    bool m_init;

    SnpeContext(const string, const std::vector<std::string>, const std::vector<std::string>, const std::vector<std::string>, const string, const string, const bool);
    void SetInputBuffer(py::array_t<float>, std::string);
    py::array_t<float> GetOutputBuffer(std::string);
    bool Execute(void);
    bool Initialize(void);
};
#endif