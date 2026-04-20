//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#pragma once

#include <iostream>
#include <map>
#include <queue>
#include <regex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "SpeechToImageApp.hpp"

namespace qnn {
namespace tools {
namespace speech_to_image {

enum class ProfilingLevel { OFF, BASIC, DETAILED, INVALID };

using ReadInputListRetType_t = std::
    tuple<std::vector<std::vector<std::string>>, std::unordered_map<std::string, uint32_t>, bool>;

ReadInputListRetType_t readInputList(std::string inputFileListPath);

using ReadInputListsRetType_t = std::tuple<std::vector<std::vector<std::vector<std::string>>>,
                                           std::vector<std::unordered_map<std::string, uint32_t>>,
                                           bool>;

ReadInputListsRetType_t readInputLists(std::vector<std::string> inputFileListPath);

std::unordered_map<std::string, uint32_t> extractInputNameIndices(const std::string &inputLine,
                                                                  const std::string &separator);

std::string sanitizeTensorName(std::string name);

ProfilingLevel parseProfilingLevel(std::string profilingLevelString);

void parseInputFilePaths(std::vector<std::string> &inputFilePaths,
                         std::vector<std::string> &paths,
                         std::string separator);

void split(std::vector<std::string> &splitString,
           const std::string &tokenizedString,
           const char separator);

bool copyMetadataToGraphsInfo(int numberOfBins,
                                          const QnnSystemContext_BinaryInfo_t *binaryInfo[],
                                          qnn_wrapper_api::GraphInfo_t **&graphsInfo,
                                          uint32_t &graphsCount);

bool copyGraphsInfo(int numberOfBins, 
                                const QnnSystemContext_BinaryInfo_t *binaryInfo[],
                                qnn_wrapper_api::GraphInfo_t **&graphsInfo,
                                uint32_t &graphsCount);

bool copyGraphsInfoV1(const QnnSystemContext_GraphInfoV1_t *graphInfoSrc,
                      qnn_wrapper_api::GraphInfo_t *graphInfoDst);

bool copyGraphsInfoV3(const QnnSystemContext_GraphInfoV3_t *graphInfoSrc, qnn_wrapper_api::GraphInfo_t *graphInfoDst);

bool copyTensorsInfo(const Qnn_Tensor_t *tensorsInfoSrc,
                     Qnn_Tensor_t *&tensorWrappers,
                     uint32_t tensorsCount);

bool deepCopyQnnTensorInfo(Qnn_Tensor_t *dst, const Qnn_Tensor_t *src);

QnnLog_Level_t parseLogLevel(std::string logLevelString);

void inline exitWithMessage(std::string &&msg, int code) {
  std::cerr << msg << std::endl;
  std::exit(code);
}

}  // namespace speech_to_image
}  // namespace tools
}  // namespace qnn