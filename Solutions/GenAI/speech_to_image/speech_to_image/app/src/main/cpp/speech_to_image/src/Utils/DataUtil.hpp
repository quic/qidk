//==============================================================================
//
//  Copyright (c) 2025 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

#pragma once

#include <map>
#include <queue>
#include <vector>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include "QnnTypes.h"

namespace qnn {
namespace tools {
namespace datautil {

enum class StatusCode {
  SUCCESS,
  DATA_READ_FAIL,
  DATA_WRITE_FAIL,
  FILE_OPEN_FAIL,
  DIRECTORY_CREATE_FAIL,
  INVALID_DIMENSIONS,
  INVALID_DATA_TYPE,
  DATA_SIZE_MISMATCH,
  INVALID_BUFFER,
};

const size_t g_bitsPerByte = 8;
std::unordered_map<std::string, int> static encoder;
std::unordered_map<int, std::string> static decoder;
std::string static unk_token;


std::vector<int32_t> tokenize(std::string& text);

void generateNormalNumber(float mean, float stddev, int size, float *output);

void get_timestep_embedding(int timestep, int embedding_dim, float timesteps_embeded[]);
std::vector<float> run_scheduler(Qnn_Tensor_t noise_pred_uncond,
              Qnn_ClientBuffer_t noise_pred_cond,
              Qnn_Tensor_t latent_in,
              int timestep);
void initialize_scheduler();
std::vector<float> step(int step, std::vector<float> sample, std::vector<float> model_output);

using ReadBatchDataRetType_t = std::tuple<StatusCode, size_t, size_t>;

std::tuple<StatusCode, size_t> getDataTypeSizeInBytes(Qnn_DataType_t dataType);

std::tuple<StatusCode, size_t> calculateLength(std::vector<size_t> dims, Qnn_DataType_t dataType);

size_t calculateElementCount(std::vector<size_t> dims);

std::tuple<StatusCode, size_t> getFileSize(std::string filePath);

StatusCode readDataFromFile(std::string filePath,
                            std::vector<size_t> dims,
                            Qnn_DataType_t dataType,
                            uint8_t* buffer);

StatusCode readBinaryFromFile(std::string filePath, uint8_t* buffer, size_t bufferSize);

StatusCode writeDataToFile(std::string fileDir,
                           std::string fileName,
                           std::vector<size_t> dims,
                           Qnn_DataType_t dataType,
                           uint8_t* buffer);

StatusCode writeBatchDataToFile(std::vector<std::string> fileDirs,
                                std::string fileName,
                                std::vector<size_t> dims,
                                Qnn_DataType_t dataType,
                                uint8_t* buffer);

StatusCode writeBinaryToFile(std::string fileDir,
                             std::string fileName,
                             uint8_t* buffer,
                             size_t bufferSize);

template <typename T_QuantType>
datautil::StatusCode floatToTfN(
    T_QuantType* out, float* in, int32_t offset, float scale, size_t numElements);

template <typename T_QuantType>
datautil::StatusCode tfNToFloat(
    float* out, T_QuantType* in, int32_t offset, float scale, size_t numElements);

template <typename T_QuantType>
datautil::StatusCode castToFloat(float* out, T_QuantType* in, size_t numElements);

template <typename T_QuantType>
datautil::StatusCode castFromFloat(T_QuantType* out, float* in, size_t numElements);

const std::map<Qnn_DataType_t, size_t> g_dataTypeToSize = {
    {QNN_DATATYPE_INT_8, 1},
    {QNN_DATATYPE_INT_16, 2},
    {QNN_DATATYPE_INT_32, 4},
    {QNN_DATATYPE_INT_64, 8},
    {QNN_DATATYPE_UINT_8, 1},
    {QNN_DATATYPE_UINT_16, 2},
    {QNN_DATATYPE_UINT_32, 4},
    {QNN_DATATYPE_UINT_64, 8},
    {QNN_DATATYPE_FLOAT_16, 2},
    {QNN_DATATYPE_FLOAT_32, 4},
    {QNN_DATATYPE_FLOAT_64, 8},
    {QNN_DATATYPE_SFIXED_POINT_8, 1},
    {QNN_DATATYPE_SFIXED_POINT_16, 2},
    {QNN_DATATYPE_SFIXED_POINT_32, 4},
    {QNN_DATATYPE_UFIXED_POINT_8, 1},
    {QNN_DATATYPE_UFIXED_POINT_16, 2},
    {QNN_DATATYPE_UFIXED_POINT_32, 4},
    {QNN_DATATYPE_BOOL_8, 1},
};

}  // namespace datautil
}  // namespace tools
}  // namespace qnn
