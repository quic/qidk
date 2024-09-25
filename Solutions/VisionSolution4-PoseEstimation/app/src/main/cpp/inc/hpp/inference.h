//============================================================================
// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

//
// Created by shubpate on 12/11/2021.
//

#ifndef NATIVEINFERENCE_INFERENCE_H
#define NATIVEINFERENCE_INFERENCE_H

#include "zdl/DlSystem/TensorShape.hpp"
#include "zdl/DlSystem/TensorMap.hpp"
#include "zdl/DlSystem/TensorShapeMap.hpp"
#include "zdl/DlSystem/IUserBufferFactory.hpp"
#include "zdl/DlSystem/IUserBuffer.hpp"
#include "zdl/DlSystem/UserBufferMap.hpp"
#include "zdl/DlSystem/IBufferAttributes.hpp"

#include "zdl/DlSystem/StringList.hpp"

#include "zdl/SNPE/SNPE.hpp"
#include "zdl/SNPE/SNPEFactory.hpp"
#include "zdl/DlSystem/DlVersion.hpp"
#include "zdl/DlSystem/DlEnums.hpp"
#include "zdl/DlSystem/String.hpp"
#include "zdl/DlContainer/IDlContainer.hpp"
#include "zdl/SNPE/SNPEBuilder.hpp"

#include "zdl/DlSystem/ITensor.hpp"
#include "zdl/DlSystem/ITensorFactory.hpp"

#include <unordered_map>
#include "android/log.h"
#include <opencv2/opencv.hpp>

#define  LOG_TAG    "SNPE_INF"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

//These 4 values are fixed for HRNET model
const int HRNET_model_output_width = 64;
const int HRNET_model_output_height = 48;
const int HRNET_model_input_width = 256;
const int HRNET_model_input_height = 192;

class BoxCornerEncoding {

public:
    int x1;
    int y1;
    int x2;
    int y2;
    float score;
    std::string objlabel;

    BoxCornerEncoding(int a, int b, int c, int d,int sc, std::string name="person")
    {
        x1 = a;
        y1 = b;
        x2 = c;
        y2 = d;
        score = sc;
        objlabel = name;
    }
};

//std::string build_network(const uint8_t * dlc_buffer, const size_t dlc_size, const uint8_t * dlc_buffer2, const size_t dlc_size2, const char runtime_arg);
std::string build_network_BB(const uint8_t * dlc_buffer, const size_t dlc_size, const char runtime_arg);
std::string build_network_pose(const uint8_t * dlc_buffer, const size_t dlc_size, const char runtime_arg);
bool SetAdspLibraryPath(std::string nativeLibPath);

bool saveOutput (zdl::DlSystem::UserBufferMap& outputMap,
                 std::unordered_map<std::string,std::vector<uint8_t>>& applicationOutputBuffers,
                 std::vector<float *> & outputVec,
                 size_t batchSize,
                 bool isTfNBuffer,
                 int bitWidth);

float*** execcomb(cv::Mat &img, int orig_width, int orig_height, int &numberofhuman, std::vector<std::vector<float>> &BB_coords);
bool execcomb(cv::Mat &img, int orig_width, int orig_height, int &numberofhuman, std::vector<std::vector<float>> &BB_coords, std::vector<std::string> &BB_names);

#endif //NATIVEINFERENCE_INFERENCE_H
