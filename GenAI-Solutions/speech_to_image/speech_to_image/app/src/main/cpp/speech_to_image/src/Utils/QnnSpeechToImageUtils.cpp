//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================


#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>

#include "Logger.hpp"
#include "PAL/Directory.hpp"
#include "PAL/FileOp.hpp"
#include "PAL/Path.hpp"
#include "PAL/StringOp.hpp"
#include "QnnSpeechToImageUtils.hpp"
#include "QnnTypeMacros.hpp"

using namespace qnn;
using namespace qnn::tools;

void speech_to_image::split(std::vector<std::string> &splitString,
                       const std::string &tokenizedString,
                       const char separator) {
  splitString.clear();
  std::istringstream tokenizedStringStream(tokenizedString);
  while (!tokenizedStringStream.eof()) {
    std::string value;
    getline(tokenizedStringStream, value, separator);
    if (!value.empty()) {
      splitString.push_back(value);
    }
  }
}

void speech_to_image::parseInputFilePaths(std::vector<std::string> &inputFilePaths,
                                     std::vector<std::string> &paths,
                                     std::string separator) {
  for (auto &inputInfo : inputFilePaths) {
    auto position = inputInfo.find(separator);
    if (position != std::string::npos) {
      auto path = inputInfo.substr(position + separator.size());
      paths.push_back(path);
    } else {
      paths.push_back(inputInfo);
    }
  }
}

speech_to_image::ReadInputListsRetType_t speech_to_image::readInputLists(
    std::vector<std::string> inputFileListPaths) {
  std::vector<std::vector<std::vector<std::string>>> filePathsLists;
  std::vector<std::unordered_map<std::string, uint32_t>> inputNameToIndexMaps;
  for (auto const &path : inputFileListPaths) {
    bool readSuccess;
    std::vector<std::vector<std::string>> filePathList;
    std::unordered_map<std::string, uint32_t> inputNameToIndex;
    std::tie(filePathList, inputNameToIndex, readSuccess) = readInputList(path);
    if (!readSuccess) {
      filePathsLists.clear();
      return std::make_tuple(filePathsLists, inputNameToIndexMaps, false);
    }
    filePathsLists.push_back(filePathList);
    inputNameToIndexMaps.push_back(inputNameToIndex);
  }
  return std::make_tuple(filePathsLists, inputNameToIndexMaps, true);
}

speech_to_image::ReadInputListRetType_t speech_to_image::readInputList(const std::string inputFileListPath) {
  std::queue<std::string> lines;
  std::ifstream fileListStream(inputFileListPath);
  if (!fileListStream) {
    QNN_ERROR("Failed to open input file: %s", inputFileListPath.c_str());
    return std::make_tuple(std::vector<std::vector<std::string>>{},
                           std::unordered_map<std::string, uint32_t>{},
                           false);
  }

  std::string fileLine;
  while (std::getline(fileListStream, fileLine)) {
    if (fileLine.empty()) continue;
    lines.push(fileLine);
  }

  if (!lines.empty() && lines.front().compare(0, 1, "#") == 0) {
    lines.pop();
  }

  if (!lines.empty() && lines.front().compare(0, 1, "%") == 0) {
    lines.pop();
  }

  std::string separator = ":=";
  std::vector<std::vector<std::string>> filePathsList;
  std::unordered_map<std::string, uint32_t> inputNameToIndex;
  if (!lines.empty()) {
    inputNameToIndex = extractInputNameIndices(lines.front(), separator);
  }
  while (!lines.empty()) {
    std::vector<std::string> paths{};
    std::vector<std::string> inputFilePaths;
    split(inputFilePaths, lines.front(), ' ');
    parseInputFilePaths(inputFilePaths, paths, separator);
    filePathsList.reserve(paths.size());
    for (size_t idx = 0; idx < paths.size(); idx++) {
      if (idx >= filePathsList.size()) {
        filePathsList.push_back(std::vector<std::string>());
      }
      filePathsList[idx].push_back(paths[idx]);
    }
    lines.pop();
  }
  return std::make_tuple(filePathsList, inputNameToIndex, true);
}

std::unordered_map<std::string, uint32_t> speech_to_image::extractInputNameIndices(
    const std::string &inputLine, const std::string &separator) {
  std::vector<std::string> inputFilePaths;
  std::unordered_map<std::string, uint32_t> inputNameToIndex;
  split(inputFilePaths, inputLine, ' ');
  size_t inputCount = 0;
  for (uint32_t idx = 0; idx < inputFilePaths.size(); idx++) {
    auto position = inputFilePaths[idx].find(separator);
    if (position != std::string::npos) {
      auto unsanitizedTensorName = inputFilePaths[idx].substr(0, position);
      auto sanitizedTensorName   = sanitizeTensorName(unsanitizedTensorName);
      if (sanitizedTensorName != unsanitizedTensorName) {
        inputNameToIndex[unsanitizedTensorName] = idx;
      }
      inputNameToIndex[sanitizedTensorName] = idx;
      inputCount                            = inputCount + 1;
    }
  }
  return inputCount == inputFilePaths.size() ? inputNameToIndex
                                             : std::unordered_map<std::string, uint32_t>{};
}

std::string speech_to_image::sanitizeTensorName(std::string name) {
  std::string sanitizedName = std::regex_replace(name, std::regex("\\W+"), "_");
  if (!std::isalpha(sanitizedName[0]) && sanitizedName[0] != '_') {
    sanitizedName = "_" + sanitizedName;
  }
  return sanitizedName;
}

speech_to_image::ProfilingLevel speech_to_image::parseProfilingLevel(std::string profilingLevelString) {
  std::transform(profilingLevelString.begin(),
                 profilingLevelString.end(),
                 profilingLevelString.begin(),
                 ::tolower);
  ProfilingLevel parsedProfilingLevel = ProfilingLevel::INVALID;
  if (profilingLevelString == "off") {
    parsedProfilingLevel = ProfilingLevel::OFF;
  } else if (profilingLevelString == "basic") {
    parsedProfilingLevel = ProfilingLevel::BASIC;
  } else if (profilingLevelString == "detailed") {
    parsedProfilingLevel = ProfilingLevel::DETAILED;
  }
  return parsedProfilingLevel;
}

bool speech_to_image::deepCopyQnnTensorInfo(Qnn_Tensor_t *dst, const Qnn_Tensor_t *src) {
  if (nullptr == dst || nullptr == src) {
    QNN_ERROR("Received nullptr");
    return false;
  }
  // set tensor.version before using QNN_TENSOR_SET macros, as they require the version to be set
  // to correctly assign values
  dst->version           = src->version;
  const char *tensorName = QNN_TENSOR_GET_NAME(src);
  if (!tensorName) {
    QNN_TENSOR_SET_NAME(dst, nullptr);
  } else {
    QNN_TENSOR_SET_NAME(dst, pal::StringOp::strndup(tensorName, strlen(tensorName)));
  }
  QNN_TENSOR_SET_ID(dst, QNN_TENSOR_GET_ID(src));
  QNN_TENSOR_SET_TYPE(dst, QNN_TENSOR_GET_TYPE(src));
  QNN_TENSOR_SET_DATA_FORMAT(dst, QNN_TENSOR_GET_DATA_FORMAT(src));
  QNN_TENSOR_SET_DATA_TYPE(dst, QNN_TENSOR_GET_DATA_TYPE(src));
  Qnn_QuantizeParams_t qParams = QNN_QUANTIZE_PARAMS_INIT;
  qParams.encodingDefinition   = QNN_TENSOR_GET_QUANT_PARAMS(src).encodingDefinition;
  qParams.quantizationEncoding = QNN_QUANTIZATION_ENCODING_UNDEFINED;
  if (QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding ==
      QNN_QUANTIZATION_ENCODING_SCALE_OFFSET) {
    qParams.quantizationEncoding = QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding;
    qParams.scaleOffsetEncoding  = QNN_TENSOR_GET_QUANT_PARAMS(src).scaleOffsetEncoding;
  } else if (QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding ==
             QNN_QUANTIZATION_ENCODING_AXIS_SCALE_OFFSET) {
    qParams.quantizationEncoding = QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding;
    qParams.axisScaleOffsetEncoding.axis =
        QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.axis;
    qParams.axisScaleOffsetEncoding.numScaleOffsets =
        QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets;
    if (QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets > 0) {
      qParams.axisScaleOffsetEncoding.scaleOffset = (Qnn_ScaleOffset_t *)malloc(
          QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets *
          sizeof(Qnn_ScaleOffset_t));
      if (qParams.axisScaleOffsetEncoding.scaleOffset) {
        for (size_t idx = 0;
             idx < QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets;
             idx++) {
          qParams.axisScaleOffsetEncoding.scaleOffset[idx].scale =
              QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.scaleOffset[idx].scale;
          qParams.axisScaleOffsetEncoding.scaleOffset[idx].offset =
              QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.scaleOffset[idx].offset;
        }
      }
    }
  }
  QNN_TENSOR_SET_QUANT_PARAMS(dst, qParams);
  QNN_TENSOR_SET_RANK(dst, QNN_TENSOR_GET_RANK(src));
  QNN_TENSOR_SET_DIMENSIONS(dst, nullptr);
  if (QNN_TENSOR_GET_RANK(src) > 0) {
    QNN_TENSOR_SET_DIMENSIONS(dst, (uint32_t *)malloc(QNN_TENSOR_GET_RANK(src) * sizeof(uint32_t)));
    if (QNN_TENSOR_GET_DIMENSIONS(dst)) {
      pal::StringOp::memscpy(QNN_TENSOR_GET_DIMENSIONS(dst),
                             QNN_TENSOR_GET_RANK(src) * sizeof(uint32_t),
                             QNN_TENSOR_GET_DIMENSIONS(src),
                             QNN_TENSOR_GET_RANK(src) * sizeof(uint32_t));
    }
  }
  return true;
}

bool speech_to_image::copyTensorsInfo(const Qnn_Tensor_t *tensorsInfoSrc,
                                 Qnn_Tensor_t *&tensorWrappers,
                                 uint32_t tensorsCount) {
  QNN_FUNCTION_ENTRY_LOG;
  auto returnStatus = true;
  tensorWrappers    = (Qnn_Tensor_t *)calloc(tensorsCount, sizeof(Qnn_Tensor_t));
  if (nullptr == tensorWrappers) {
    QNN_ERROR("Failed to allocate memory for tensorWrappers.");
    return false;
  }
  if (returnStatus) {
    for (size_t tIdx = 0; tIdx < tensorsCount; tIdx++) {
      QNN_DEBUG("Extracting tensorInfo for tensor Idx: %d", tIdx);
      tensorWrappers[tIdx] = QNN_TENSOR_INIT;
      deepCopyQnnTensorInfo(&tensorWrappers[tIdx], &tensorsInfoSrc[tIdx]);
    }
  }
  QNN_FUNCTION_EXIT_LOG;
  return returnStatus;
}

bool speech_to_image::copyGraphsInfoV3(const QnnSystemContext_GraphInfoV3_t *graphInfoSrc,
                                  qnn_wrapper_api::GraphInfo_t *graphInfoDst) {
  graphInfoDst->graphName = nullptr;
  if (graphInfoSrc->graphName) {
    graphInfoDst->graphName =
        pal::StringOp::strndup(graphInfoSrc->graphName, strlen(graphInfoSrc->graphName));
  }
  graphInfoDst->inputTensors    = nullptr;
  graphInfoDst->numInputTensors = 0;
  if (graphInfoSrc->graphInputs) {
    if (!copyTensorsInfo(
            graphInfoSrc->graphInputs, graphInfoDst->inputTensors, graphInfoSrc->numGraphInputs)) {
      return false;
    }
    graphInfoDst->numInputTensors = graphInfoSrc->numGraphInputs;
  }
  graphInfoDst->outputTensors    = nullptr;
  graphInfoDst->numOutputTensors = 0;
  if (graphInfoSrc->graphOutputs) {
    if (!copyTensorsInfo(graphInfoSrc->graphOutputs,
                         graphInfoDst->outputTensors,
                         graphInfoSrc->numGraphOutputs)) {
      return false;
    }
    graphInfoDst->numOutputTensors = graphInfoSrc->numGraphOutputs;
  }
  return true;
}

bool speech_to_image::copyGraphsInfoV1(const QnnSystemContext_GraphInfoV1_t *graphInfoSrc,
                                  qnn_wrapper_api::GraphInfo_t *graphInfoDst) {
  graphInfoDst->graphName = nullptr;
  if (graphInfoSrc->graphName) {
    graphInfoDst->graphName =
        pal::StringOp::strndup(graphInfoSrc->graphName, strlen(graphInfoSrc->graphName));
        QNN_INFO("speech_to_image -> Inner. graphInfoSrc->graphName: %s", graphInfoSrc->graphName);
  }
  graphInfoDst->inputTensors    = nullptr;
  graphInfoDst->numInputTensors = 0;
  if (graphInfoSrc->graphInputs) {
    if (!copyTensorsInfo(
            graphInfoSrc->graphInputs, graphInfoDst->inputTensors, graphInfoSrc->numGraphInputs)) {
      return false;
    }
    graphInfoDst->numInputTensors = graphInfoSrc->numGraphInputs;
  }
  graphInfoDst->outputTensors    = nullptr;
  graphInfoDst->numOutputTensors = 0;
  if (graphInfoSrc->graphOutputs) {
    if (!copyTensorsInfo(graphInfoSrc->graphOutputs,
                         graphInfoDst->outputTensors,
                         graphInfoSrc->numGraphOutputs)) {
      return false;
    }
    graphInfoDst->numOutputTensors = graphInfoSrc->numGraphOutputs;
  }
  return true;
}


bool speech_to_image::copyGraphsInfo(int numberOfBins, 
                                const QnnSystemContext_BinaryInfo_t *binaryInfo[],
                                qnn_wrapper_api::GraphInfo_t **&graphsInfo,
                                uint32_t &graphsCount) {
  graphsCount = 0;
  QNN_FUNCTION_ENTRY_LOG;
  for(int i=0; i<numberOfBins; i++){
    QNN_INFO("Binary vers %d: %d", i, binaryInfo[i]->version );

    if (!binaryInfo[i]->contextBinaryInfoV3.graphs) {
      QNN_ERROR("Received nullptr for graphsInput.");
      return false;
    }
    graphsCount += binaryInfo[i]->contextBinaryInfoV3.numGraphs;

  }

  QNN_INFO("Total number of graphs %d", graphsCount);
  
  auto returnStatus = true;
  graphsInfo =
      (qnn_wrapper_api::GraphInfo_t **)calloc(graphsCount, sizeof(qnn_wrapper_api::GraphInfo_t *));
  qnn_wrapper_api::GraphInfo_t *graphInfoArr =
      (qnn_wrapper_api::GraphInfo_t *)calloc(graphsCount, sizeof(qnn_wrapper_api::GraphInfo_t));
  if (nullptr == graphsInfo || nullptr == graphInfoArr) {
    QNN_ERROR("Failure to allocate memory for *graphInfo");
    returnStatus = false;
  }
  size_t graphInfoIndex = 0;
  if (true == returnStatus) {
    for(int i=0; i<numberOfBins; i++){
      for (size_t gIdx = 0; gIdx < binaryInfo[i]->contextBinaryInfoV3.numGraphs; gIdx++) {
        QNN_INFO("speech_to_image -> Extracting graphsInfo for bin %d, graph Idx: %d", i, gIdx);
        if (binaryInfo[i]->contextBinaryInfoV3.graphs[gIdx].version == QNN_SYSTEM_CONTEXT_GRAPH_INFO_VERSION_3) {
          QNN_INFO("speech_to_image Outer -> Copying the info. graphInfoIndex: %d, gIdx: %d, graphname: %s", graphInfoIndex, gIdx, binaryInfo[graphInfoIndex]->contextBinaryInfoV3.graphs[gIdx].graphInfoV3.graphName);
          copyGraphsInfoV3(&(binaryInfo[graphInfoIndex]->contextBinaryInfoV3.graphs[gIdx].graphInfoV3), &graphInfoArr[graphInfoIndex]);
        }
        graphsInfo[graphInfoIndex] = graphInfoArr + graphInfoIndex;
        QNN_INFO("speech_to_image -> GRAPH-INFO %s. Index: %d", graphsInfo[graphInfoIndex]->graphName, graphInfoIndex);
        graphInfoIndex++;
      }
    }
  }
  graphInfoIndex = 0;
  if (true != returnStatus) {
    QNN_ERROR("Received an ERROR during extractGraphsInfo. Freeing resources.");
    if (graphsInfo) {
      for(int i=0; i<numberOfBins; i++){
        for (uint32_t gIdx = 0; gIdx < binaryInfo[i]->contextBinaryInfoV1.numGraphs; gIdx++) {
          if (graphsInfo[graphInfoIndex]) {
            if (nullptr != graphsInfo[graphInfoIndex]->graphName) {
              free(graphsInfo[graphInfoIndex]->graphName);
              graphsInfo[graphInfoIndex]->graphName = nullptr;
            }
            
            qnn_wrapper_api::freeQnnTensors(graphsInfo[graphInfoIndex]->inputTensors,
                                            graphsInfo[graphInfoIndex]->numInputTensors);
            qnn_wrapper_api::freeQnnTensors(graphsInfo[graphInfoIndex]->outputTensors,
                                            graphsInfo[graphInfoIndex]->numOutputTensors);
          }
          graphInfoIndex++;
        }
      }
      free(*graphsInfo);
    }
    free(graphsInfo);
    graphsInfo = nullptr;
  }
  QNN_FUNCTION_EXIT_LOG;
  return true;
}
bool speech_to_image::copyMetadataToGraphsInfo(int numberOfBins,
                                          const QnnSystemContext_BinaryInfo_t *binaryInfo[],
                                          qnn_wrapper_api::GraphInfo_t **&graphsInfo,
                                          uint32_t &graphsCount) {
  for(int i=0; i<numberOfBins; i++){
    if (nullptr == binaryInfo[i]) {
      QNN_ERROR("One of the binaryInfo is nullptr.");
      return false;
    }
  }

  if (!copyGraphsInfo(numberOfBins, binaryInfo, graphsInfo, graphsCount)) {
      QNN_ERROR("Failed while copying graphs Info.");
      return false;
  }
  return true;
}

QnnLog_Level_t speech_to_image::parseLogLevel(std::string logLevelString) {
  QNN_FUNCTION_ENTRY_LOG;
  std::transform(logLevelString.begin(), logLevelString.end(), logLevelString.begin(), ::tolower);
  QnnLog_Level_t parsedLogLevel = QNN_LOG_LEVEL_MAX;
  if (logLevelString == "error") {
    parsedLogLevel = QNN_LOG_LEVEL_ERROR;
  } else if (logLevelString == "warn") {
    parsedLogLevel = QNN_LOG_LEVEL_WARN;
  } else if (logLevelString == "info") {
    parsedLogLevel = QNN_LOG_LEVEL_INFO;
  } else if (logLevelString == "verbose") {
    parsedLogLevel = QNN_LOG_LEVEL_VERBOSE;
  } else if (logLevelString == "debug") {
    parsedLogLevel = QNN_LOG_LEVEL_DEBUG;
  }
  QNN_FUNCTION_EXIT_LOG;
  return parsedLogLevel;
}

