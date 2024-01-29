// -*- mode: cpp -*-
// =============================================================================
// @@-COPYRIGHT-START-@@
//
// Copyright (c) 2023 of Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause
//
// @@-COPYRIGHT-END-@@
// =============================================================================
#include "snpehelper.h"

/** @brief Constructor
* @param model_path for accessing .dlc 
* @param input_layers conatins list of input layers to the model
* @param output_layers conatins list of output layers of the model
* @param output_tensors conatins list of output tensors of the model
* @param run_time for execution on hardware CPU/DSP
* @param profile_level to set performance
* @param enable_cache to store online preparation of the graph
*/

SnpeContext::SnpeContext(const string Model_Path, const std::vector<std::string> Input_Layers, const std::vector<std::string> Output_Layers, const std::vector<std::string> Output_Tensors, const string Run_Time, const string Profile_Level, const bool Enable_Cache)
{
    m_model_path = Model_Path;
    m_inputLayers = Input_Layers;
    m_outputLayers = Output_Layers;
    m_outputTensors = Output_Tensors;
    m_enable_init_cache = Enable_Cache;

    // To set profiling level
    if (Profile_Level.compare("EXTREME_POWERSAVER") == 0)
    {
        m_profiling_level = EXTREME_POWERSAVER;
    }
    else if (Profile_Level.compare("HIGH_PERFORMANCE") == 0)
    {
        m_profiling_level = HIGH_PERFORMANCE;
    }
    else if (Profile_Level.compare("POWER_SAVER") == 0)
    {
        m_profiling_level = POWER_SAVER;
    }
    else if (Profile_Level.compare("SYSTEM_SETTINGS") == 0)
    {
        m_profiling_level = SYSTEM_SETTINGS;
    }
    else if (Profile_Level.compare("SUSTAINED_HIGH_PERFORMANCE") == 0)
    {
        m_profiling_level = SUSTAINED_HIGH_PERFORMANCE;
    }
    else if (Profile_Level.compare("BURST") == 0)
    {
        m_profiling_level = BURST;
    }
    else if (Profile_Level.compare("LOW_POWER_SAVER") == 0)
    {
        m_profiling_level = LOW_POWER_SAVER;
    }
    else if (Profile_Level.compare("HIGH_POWER_SAVER") == 0)
    {
        m_profiling_level = HIGH_POWER_SAVER;
    }
    else if (Profile_Level.compare("LOW_BALANCED") == 0)
    {
        m_profiling_level = LOW_BALANCED;
    }
    else
    {
        m_profiling_level = BALANCED;
    }

    // To set runtime 
    if (Run_Time.compare("DSP") == 0)
    {
        m_runtime = DSP;
    }
    else
    {
        m_runtime = CPU;
    }

    for (int i = 0; i < m_outputTensors.size(); i++)
    {
        map_m_outputTensors.insert(std::make_pair(m_outputTensors[i], i));
    }

    for (int i = 0; i < m_inputLayers.size(); i++)
    {
        map_m_inputLayers.insert(std::make_pair(m_inputLayers[i], i));
    }

    m_init = true;
    input_tensor = NULL;
    output_tensor = NULL;
}

/** @brief To allocate buffers for snpe runtime
* @return true if success; false otherwise
*/
bool SnpeContext::Initialize(void)
{
#ifdef DEBUG
    auto start1 = chrono::steady_clock::now();
#endif
    //LOG_INFO("Initializing SNPE instance\n");

    m_snperuntime = std::move(std::unique_ptr<snperuntime::SNPERuntime>(new snperuntime::SNPERuntime()));

    m_snperuntime->SetOutputLayers(m_outputLayers);
    if (!m_snperuntime->Initialize(m_model_path, m_runtime, m_profiling_level, m_enable_init_cache))
    {
        LOG_ERROR("Failed to  init SNPE instance\n");
        m_init = false;
    }
    if (m_init == true)
    {
        for (int i = 0; i < m_inputLayers.size(); i++)
        {
            if (m_snperuntime->GetInputTensor(m_inputLayers[i]) == NULL)
            {
                LOG_ERROR("Empty input tensor\n");
                m_init = false;
            }
        }
        for (int i = 0; i < m_outputTensors.size(); i++)
        {
            if (m_snperuntime->GetOutputTensor(m_outputTensors[i]) == NULL)
            {
                LOG_ERROR("Empty output tensor\n");
                m_init = false;
            }
        }
    }
#ifdef DEBUG
    auto end1 = chrono::steady_clock::now();
    float costTime1 = chrono::duration_cast<chrono::microseconds>(end1 - start1).count();
    LOG_DEBUG("Elapsed Initialize time in milliseconds: %lf ms\n", costTime1 / 1000);
#endif
    return m_init;
}

/** @brief To set Input buffer
* @param Input_Data 
* @param Input_Layer to overwrite Input_Data for specified layer
*/
void SnpeContext::SetInputBuffer(py::array_t<float> Input_Data, std::string Input_Layer)
{
#ifdef DEBUG
    auto start1 = chrono::steady_clock::now();
#endif
    auto inputShape = m_snperuntime->GetInputShape(m_inputLayers[map_m_inputLayers[Input_Layer]]);

    input_tensor = m_snperuntime->GetInputTensor(m_inputLayers[map_m_inputLayers[Input_Layer]]);
    auto buf = Input_Data.request();
    float* input_image = (float*)buf.ptr;

    size_t input_size = 1;
    size_t number_of_dim = inputShape.size();
    for (size_t i = 0; i < number_of_dim; i++)
        input_size *= inputShape[i];
    memcpy(input_tensor, input_image, input_size * sizeof(float));

#ifdef DEBUG
    auto end1 = chrono::steady_clock::now();
    float costTime1 = chrono::duration_cast<chrono::microseconds>(end1 - start1).count();
    LOG_DEBUG("Elapsed SetInputBuffer time in milliseconds: %f ms\n", costTime1 / 1000);
#endif
}

/** @brief To execute on Qualcomm hardware
* @return true if success;false otherwise
*/
bool SnpeContext::Execute(void)
{
#ifdef DEBUG
    auto start1 = chrono::steady_clock::now();
#endif
    if (m_init != true)
    {
        return false;
    }
    //LOG_INFO("Executing %s\n",m_model_path.c_str());

    if (!m_snperuntime->execute())
    {
        LOG_ERROR("SNPERuntime execute failed\n");
        return false;
    }
#ifdef DEBUG
    auto end1 = chrono::steady_clock::now();
    float costTime1 = chrono::duration_cast<chrono::microseconds>(end1 - start1).count();
    LOG_DEBUG("Elapsed Execute time in milliseconds: %f ms\n", costTime1 / 1000);
#endif
    return true;
}

/** @brief To retrieve data from output node
* @param Output_Tensor specifies output node
* @return array_t which is considered as numpy array
*/
py::array_t<float> SnpeContext::GetOutputBuffer(std::string Output_Tensor)
{
#ifdef DEBUG
    auto start1 = chrono::steady_clock::now();
#endif

    auto outputShape = m_snperuntime->GetOutputShape(m_outputTensors[map_m_outputTensors[Output_Tensor]]);
    float* predOutput = m_snperuntime->GetOutputTensor(m_outputTensors[map_m_outputTensors[Output_Tensor]]);

    size_t output_size = 1;
    size_t number_of_dim = outputShape.size();

    for (size_t i = 0; i < number_of_dim; i++)
        output_size *= outputShape[i];

    py::array_t<float> output(output_size);
    auto buf = output.request();
    float* ptr = (float*)buf.ptr;
    memcpy(ptr, predOutput, output_size * sizeof(float));

#ifdef DEBUG 
    auto end1 = chrono::steady_clock::now();
    float costTime1 = chrono::duration_cast<chrono::microseconds>(end1 - start1).count();
    LOG_DEBUG("Elapsed GetOutputBuffer time in milliseconds: %f\n", costTime1 / 1000);
#endif
    return output;
}
