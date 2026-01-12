//==============================================================================
//
//  Copyright (c) 2025 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

#include <cmath>
#include <ctime>
#include <fstream>
#include <iostream>
#include <numeric>
#include <queue>
#include <math.h>
#include <time.h>
#include <vector>
#include <set>
#include <algorithm>
#include <string>
#include <map>
#include <fstream>
#include <limits>

#include "DataUtil.hpp"
#include "Logger.hpp"
#include "PAL/Directory.hpp"
#include "PAL/FileOp.hpp"
#include "PAL/Path.hpp"
#include "QnnTypeMacros.hpp"
#include <random>
//#include <tokenizers_cpp.h>
//#include <sentencepiece_processor.h>


using namespace qnn;
using namespace qnn::tools;

//double num_train_timesteps = 1000;
//double beta_start = 0.00085;
//double beta_end = 0.012;
//double user_text_guidance = 7.5;

std::string timestep_spacing = "linspace";
std::string solver_type = "midpoint";
std::string beta_schedule = "scaled_linear";
std::string algorithm_type = "dpmsolver++";
std::int32_t solver_order = 2;
std::float_t beta_start = 0.00085;
std::float_t beta_end = 0.012;
std::int32_t num_train_timesteps = 1000;
std::vector<float> betas;
std::vector<float> alphas;
std::vector<float> alphas_cumprod;
std::vector<float> alpha_t;
std::vector<float> sigma_t;
std::vector<float> lambda_t;
std::float_t init_noise_sigma = 1.0;
std::vector<int> timesteps;
std::int32_t num_inference_steps = 20;
std::vector<float> sigmas;
std::int32_t lower_order_nums = 0;
std::float_t dynamic_thresholding_ratio = 0.995;
std::float_t sample_max_value = 1.0;
std::vector<std::vector<float>> model_outputs(2);
int modelSize = 16384;

extern std::string assets_path;

template <typename T>
T clamp(const T& value, const T& min_val, const T& max_val) {
    if (value < min_val) {
        return min_val;
    } else if (value > max_val) {
        return max_val;
    } else {
        return value;
    }
}

std::vector<float> dpm_solver_first_order_update(std::vector<float> x0_pred, int prev_timestep, int timestep, std::vector<float> sample){
        float lambda_t_here = lambda_t[prev_timestep];
        float lambda_s = lambda_t[timestep];
        float alpha_t_here = alpha_t[prev_timestep];
        //float alpha_s = alpha_t[timestep];
        float sigma_t_here = sigma_t[prev_timestep];
        float sigma_s = sigma_t[timestep];
        float h = lambda_t_here - lambda_s;
        std::vector<float> x_t(sample.size());
        for (size_t i = 0; i < sample.size(); ++i) {
          x_t[i] = (sigma_t_here / sigma_s) * sample[i]
                - (alpha_t_here * (std::exp(-h) - 1.0f)) * x0_pred[i];
          
        }
        return x_t;
}

// Function to compute the sample tensor at the previous timestep
std::vector<float> multistep_dpm_solver_second_order_update(
    int prev_timestep,
    const std::vector<int>& timestep_list,
    std::vector<float> sample
) {
    std::vector<float> noise = {};
    int t = prev_timestep;
    int s0 = timestep_list.back();
    int s1 = timestep_list[timestep_list.size() - 2];
    std::vector<float> m0 = model_outputs.back();
    std::vector<float> m1 = model_outputs[model_outputs.size() - 2];
    float lambda_t_here = lambda_t[t];
    float lambda_s0 = lambda_t[s0];
    float lambda_s1 = lambda_t[s1];
    float alpha_t_here = alpha_t[t];
    float sigma_t_here = sigma_t[t];
    float sigma_s0 = sigma_t[s0];
    float h = lambda_t_here - lambda_s0;
    float h_0 = lambda_s0 - lambda_s1;
    float r0 = h_0 / h;
    
    if(m0.size() == 0){
      for(size_t i=0; i< m1.size(); i++){
        m0.push_back(0);
      }
    }

    if(m1.size() == 0){
      for(size_t i=0; i< m0.size(); i++){
        m1.push_back(0);
      }
    }

    std::vector<float> D0 = m0;
    std::vector<float> D1(m0.size());
    for (size_t i = 0; i < m0.size(); ++i) {
        D1[i] = (1.0f / r0) * (m0[i] - m1[i]);
    }
    std::vector<float> x_t(sample.size());
    for (size_t i = 0; i < sample.size(); ++i) {
            x_t[i] = (sigma_t_here / sigma_s0) * sample[i]
                - (alpha_t_here * (std::exp(-h) - 1.0f)) * D0[i]
                - 0.5f * (alpha_t_here * (std::exp(-h) - 1.0f)) * D1[i];
    }
    return x_t;
}


// Function to find the index where lambda_min_clipped would be inserted
int searchsorted(std::vector<float>& lambda_t) {
    auto it = std::lower_bound(lambda_t.begin(), lambda_t.end(), -1*std::numeric_limits<float>::infinity());
    return std::distance(lambda_t.begin(), it);
}

std::vector<float> compute_cumulative_product(const std::vector<float>& alphas) {
    std::vector<float> alphas_cumprod;
    float cumulative_product = 1.0;

    for (const float alpha : alphas) {
        cumulative_product *= alpha;
        alphas_cumprod.push_back(cumulative_product);
    }

    return alphas_cumprod;
}

std::vector<float> compute_betas() {
    std::vector<float> result;

    // Calculate the square root of beta_start and beta_end
    float sqrt_beta_start = std::sqrt(beta_start);
    float sqrt_beta_end = std::sqrt(beta_end);

    // Compute the step size
    float step_size = (sqrt_beta_end - sqrt_beta_start) / (num_train_timesteps - 1);

    // Generate the values and square them
    for(int i = 0; i < num_train_timesteps; ++i) {
        float sqrt_beta = sqrt_beta_start + i * step_size;
        result.push_back(sqrt_beta * sqrt_beta);
    }

    return result;
}

void  datautil::initialize_scheduler(){

    for(int i=0; i<modelSize; i++){
      model_outputs[0].push_back(0);
      model_outputs[1].push_back(0);
    }

    betas = compute_betas();
    for(auto value:betas){
        alphas.push_back(1.0 - value);
    }
    alphas_cumprod = compute_cumulative_product(alphas);
    for (float alpha_cumprod : alphas_cumprod) {
        float alpha = std::sqrt(alpha_cumprod);
        float sigma = std::sqrt(1.0f - alpha_cumprod);
        float lambda = std::log(alpha) - std::log(sigma);

        alpha_t.push_back(alpha);
        sigma_t.push_back(sigma);
        lambda_t.push_back(lambda);
    }
    
    for (float alpha_cumprod : alphas_cumprod) {
        float sigma = std::sqrt((1.0f - alpha_cumprod) / alpha_cumprod);
        sigmas.push_back(sigma);
    }
    
    timesteps.clear();
    timesteps.push_back(999);
    timesteps.push_back(949);
    timesteps.push_back(899);
    timesteps.push_back(849);
    timesteps.push_back(799);
    timesteps.push_back(749);
    timesteps.push_back(699);
    timesteps.push_back(649);
    timesteps.push_back(599);
    timesteps.push_back(549);
    timesteps.push_back(500);
    timesteps.push_back(450);
    timesteps.push_back(400);
    timesteps.push_back(350);
    timesteps.push_back(300);
    timesteps.push_back(250);
    timesteps.push_back(200);
    timesteps.push_back(150);
    timesteps.push_back(100);
    timesteps.push_back(50);
}

std::vector<float>  datautil::step(int step, std::vector<float> sample, std::vector<float> model_output){
  int step_index = step;
  step = timesteps[step];


  std::vector<float> return_vector;
  int prev_timestep = (step_index == (int)timesteps.size() - 1) ? 0 : timesteps[step_index + 1];

  bool lower_order_final = (step_index == (int)timesteps.size() - 1) &&
                               true && (int)timesteps.size() < 15;

  float alpha_t_here = alpha_t[step];
  float sigma_t_here = sigma_t[step];


  std::vector<float> x0_pred(sample.size());
  for (size_t i = 0; i < sample.size(); ++i) {
      x0_pred[i] = (sample[i] - sigma_t_here * model_output[i]) / alpha_t_here;
      
  }
  for (int i = 0; i < solver_order - 1; ++i) {
    for(size_t j = 0; j < sample.size(); ++j){
      model_outputs[i][j] = model_outputs[i + 1][j];
    }
      
  }
  for(size_t j = 0; j < sample.size(); ++j){
    model_outputs[solver_order - 1][j] = x0_pred[j];
  }

  if(lower_order_nums < 1 || lower_order_final){
    return_vector = dpm_solver_first_order_update(x0_pred, prev_timestep, step, sample);
  }else{
    std::vector<int> timestep_list = {timesteps[step_index - 1], step};
    return_vector = multistep_dpm_solver_second_order_update(prev_timestep, timestep_list, sample);
  }

  if(lower_order_nums < solver_order){
      lower_order_nums += 1;
  }
  return return_vector;
}
 
void datautil::get_timestep_embedding(int timestep, int embedding_dim, float timesteps_embeded[]) {

    uint64_t bufferSize;
    uint8_t *buffer= nullptr;
    tools::datautil::StatusCode status{tools::datautil::StatusCode::SUCCESS};
    std::string path = assets_path + "t_emb_" + std::to_string(timestep) + ".raw";
    std::tie(status, bufferSize) = tools::datautil::getFileSize(path);
    buffer = new uint8_t[bufferSize];
    status = tools::datautil::readBinaryFromFile(path, buffer, bufferSize);

    float *ptr = reinterpret_cast<float*>(buffer);
    
    for(int i = 0; i < embedding_dim; ++i){
      timesteps_embeded[i] = *(ptr + i);
    }
}

void fillDims(std::vector<size_t>& dims,
              uint32_t* inDimensions,
              uint32_t rank) {
  if (nullptr == inDimensions) {
    QNN_ERROR("input dimensions is nullptr");
  }
  for (size_t r = 0; r < rank; r++) {
    dims.push_back(inDimensions[r]);
  }
}

std::vector<float> datautil::run_scheduler(Qnn_Tensor_t noise_pred_cond,
              Qnn_ClientBuffer_t noise_pred_uncond,
              Qnn_Tensor_t latent_in,
              int timestep){

  int lenght = QNN_TENSOR_GET_CLIENT_BUF(latent_in).dataSize/2;
  std::vector<float> tmp_buf_noise(lenght);
  std::vector<float> tmp_buf_noise_cond(lenght);
  std::vector<float> tmp_buf_noise_uncond(lenght);
  std::vector<float> tmp_buf_latent_in(lenght);

      
  datautil::tfNToFloat<uint16_t>(
              tmp_buf_noise_cond.data(),
              reinterpret_cast<uint16_t*>(QNN_TENSOR_GET_CLIENT_BUF(noise_pred_cond).data),
              QNN_TENSOR_GET_QUANT_PARAMS(noise_pred_cond).scaleOffsetEncoding.offset,
              QNN_TENSOR_GET_QUANT_PARAMS(noise_pred_cond).scaleOffsetEncoding.scale,
             QNN_TENSOR_GET_CLIENT_BUF(noise_pred_cond).dataSize/2);

  datautil::tfNToFloat<uint16_t>(
              tmp_buf_noise_uncond.data(),
              reinterpret_cast<uint16_t*>(noise_pred_uncond.data),
              QNN_TENSOR_GET_QUANT_PARAMS(noise_pred_cond).scaleOffsetEncoding.offset,
              QNN_TENSOR_GET_QUANT_PARAMS(noise_pred_cond).scaleOffsetEncoding.scale,
              QNN_TENSOR_GET_CLIENT_BUF(noise_pred_cond).dataSize/2);

  
  datautil::tfNToFloat<uint16_t>(
              tmp_buf_latent_in.data(),
              reinterpret_cast<uint16_t*>(QNN_TENSOR_GET_CLIENT_BUF(latent_in).data),
              QNN_TENSOR_GET_QUANT_PARAMS(latent_in).scaleOffsetEncoding.offset,
              QNN_TENSOR_GET_QUANT_PARAMS(latent_in).scaleOffsetEncoding.scale,
              QNN_TENSOR_GET_CLIENT_BUF(latent_in).dataSize/2);

  //noise_pred_uncond + user_text_guidance * (noise_pred_text - noise_pred_uncond)
  for(size_t i=0; i<QNN_TENSOR_GET_CLIENT_BUF(latent_in).dataSize/2; i++){
    tmp_buf_noise[i] = tmp_buf_noise_uncond[i] +  7.5*(tmp_buf_noise_cond[i] - tmp_buf_noise_uncond[i]);
  }

  return step(timestep, std::vector<float>(tmp_buf_latent_in), std::vector<float>(tmp_buf_noise));
  
}

std::string LoadBytesFromFile(const std::string& path) {
  std::ifstream fs(path, std::ios::in | std::ios::binary);
  if (fs.fail()) {
    std::cerr << "Cannot open " << path << std::endl;
    exit(1);
  }
  fs.seekg(0, std::ios::end);
  size_t size = static_cast<size_t>(fs.tellg());
  fs.seekg(0, std::ios::beg);
  char* data = new char[size];
  fs.read(data, size);
  return data;
}


std::vector<int32_t> datautil::tokenize(std::string& file) {
    std::vector<int32_t> output;
    uint64_t bufferSize;
    uint8_t *buffer= nullptr;
    std::string path = assets_path + file;
    tools::datautil::StatusCode status{tools::datautil::StatusCode::SUCCESS};
    std::tie(status, bufferSize) = tools::datautil::getFileSize(path);
    buffer = new uint8_t[bufferSize];
    status = tools::datautil::readBinaryFromFile(path, buffer, bufferSize);
    float *ptr = reinterpret_cast<float*>(buffer);
    for(size_t i = 0; i < bufferSize/4; ++i){
      output.push_back(static_cast<int32_t>(*(ptr + i)));
    }
    return output;
}

void datautil::generateNormalNumber(float mean, float stddev, int size, float * output) {
  std::time_t currentTime = std::time(nullptr);
  std::default_random_engine generator(static_cast<unsigned int>(currentTime));
  std::normal_distribution<double> distribution(mean, stddev);
  for(int i = 0; i < size; ++i){
    output[i] = (float)distribution(generator); 
  }
}

std::tuple<datautil::StatusCode, size_t> datautil::getDataTypeSizeInBytes(Qnn_DataType_t dataType) {
  if (g_dataTypeToSize.find(dataType) == g_dataTypeToSize.end()) {
    QNN_ERROR("Invalid qnn data type provided");
    return std::make_tuple(StatusCode::INVALID_DATA_TYPE, 0);
  }
  return std::make_tuple(StatusCode::SUCCESS, g_dataTypeToSize.find(dataType)->second);
}

size_t datautil::calculateElementCount(std::vector<size_t> dims) {
  if (dims.size() == 0) {
    return 0;
  }
  return std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<size_t>());
}

std::tuple<datautil::StatusCode, size_t> datautil::calculateLength(std::vector<size_t> dims,
                                                                   Qnn_DataType_t dataType) {
  if (dims.size() == 0) {
    QNN_ERROR("dims.size() is zero");
    return std::make_tuple(StatusCode::INVALID_DIMENSIONS, 0);
  }
  StatusCode returnStatus{StatusCode::SUCCESS};
  size_t length{0};
  std::tie(returnStatus, length) = getDataTypeSizeInBytes(dataType);
  if (StatusCode::SUCCESS != returnStatus) {
    return std::make_tuple(returnStatus, 0);
  }
  length *= calculateElementCount(dims);
  return std::make_tuple(StatusCode::SUCCESS, length);
}

std::tuple<datautil::StatusCode, size_t> datautil::getFileSize(std::string filePath) {
  std::ifstream in(filePath, std::ifstream::binary);
  if (!in) {
    QNN_ERROR("Failed to open input file: %s", filePath.c_str());
    return std::make_tuple(StatusCode::FILE_OPEN_FAIL, 0);
  }
  in.seekg(0, in.end);
  const size_t length = in.tellg();
  in.seekg(0, in.beg);
  return std::make_tuple(StatusCode::SUCCESS, length);
}

datautil::StatusCode datautil::readBinaryFromFile(std::string filePath,
                                                  uint8_t* buffer,
                                                  size_t bufferSize) {
  if (nullptr == buffer) {
    QNN_ERROR("buffer is nullptr");
    return StatusCode::INVALID_BUFFER;
  }
  std::ifstream in(filePath, std::ifstream::binary);
  if (!in) {
    QNN_ERROR("Failed to open input file: %s", filePath.c_str());
    return StatusCode::FILE_OPEN_FAIL;
  }
  if (!in.read(reinterpret_cast<char*>(buffer), bufferSize)) {
    QNN_ERROR("Failed to read the contents of: %s", filePath.c_str());
    return StatusCode::DATA_READ_FAIL;
  }
  return StatusCode::SUCCESS;
}

datautil::StatusCode datautil::writeDataToFile(std::string fileDir,
                                               std::string fileName,
                                               std::vector<size_t> dims,
                                               Qnn_DataType_t dataType,
                                               uint8_t* buffer) {
  if (nullptr == buffer) {
    QNN_ERROR("buffer is nullptr");
    return StatusCode::INVALID_BUFFER;
  }
  if (!pal::Directory::makePath(fileDir)) {
    QNN_ERROR("Failed to create output directory: %s", fileDir.c_str());
    return StatusCode::DIRECTORY_CREATE_FAIL;
  }
  const std::string outputPath(fileDir + pal::Path::getSeparator() + fileName);
  std::ofstream os(outputPath, std::ofstream::binary);
  if (!os) {
    QNN_ERROR("Failed to open output file for writing: %s", outputPath.c_str());
    return StatusCode::FILE_OPEN_FAIL;
  }
  StatusCode err{StatusCode::SUCCESS};
  size_t length{0};
  std::tie(err, length) = datautil::calculateLength(dims, dataType);
  if (StatusCode::SUCCESS != err) {
    return err;
  }
  for (size_t l = 0; l < length; l++) {
    os.write(reinterpret_cast<char*>(&(*(buffer + l))), 1);
  }
  return StatusCode::SUCCESS;
}

datautil::StatusCode datautil::writeBatchDataToFile(std::vector<std::string> fileDirs,
                                                    std::string fileName,
                                                    std::vector<size_t> dims,
                                                    Qnn_DataType_t dataType,
                                                    uint8_t* buffer) {
  if (nullptr == buffer) {
    QNN_ERROR("buffer is nullptr");
    return StatusCode::INVALID_BUFFER;
  }
  StatusCode err{StatusCode::SUCCESS};
  size_t length{0};
  std::tie(err, length) = datautil::calculateLength(dims, dataType);
  if (StatusCode::SUCCESS != err) {
    return err;
  }
  auto outputSize = (length);
  std::string fileDir = fileDirs[0];
    if (!pal::Directory::makePath(fileDir)) {
      QNN_INFO("Failed to create output directory: %s", fileDir.c_str());
      return StatusCode::DIRECTORY_CREATE_FAIL;
    }
    const std::string outputPath(fileDir + pal::Path::getSeparator() + fileName);
    std::ofstream os(outputPath, std::ofstream::binary);
    if (!os) {
      QNN_INFO("Failed to open output file for writing: %s", outputPath.c_str());
      return StatusCode::FILE_OPEN_FAIL;
    }
    for (size_t l = 0; l < outputSize; l++) {
      size_t bufferIndex = l + (0 * outputSize);
      os.write(reinterpret_cast<char*>(&(*(buffer + bufferIndex))), 1);
    }
  
  return StatusCode::SUCCESS;
}

datautil::StatusCode datautil::writeBinaryToFile(std::string fileDir,
                                                 std::string fileName,
                                                 uint8_t* buffer,
                                                 size_t bufferSize) {
  if (nullptr == buffer) {
    QNN_ERROR("buffer is nullptr");
    return StatusCode::INVALID_BUFFER;
  }
  if (!pal::Directory::makePath(fileDir)) {
    QNN_ERROR("Failed to create output directory: %s", fileDir.c_str());
    return StatusCode::DIRECTORY_CREATE_FAIL;
  }
  const std::string outputPath(fileDir + pal::Path::getSeparator() + fileName);
  std::ofstream os(outputPath, std::ofstream::binary);
  if (!os) {
    QNN_ERROR("Failed to open output file for writing: %s", outputPath.c_str());
    return StatusCode::FILE_OPEN_FAIL;
  }
  os.write(reinterpret_cast<char*>(buffer), bufferSize);
  return StatusCode::SUCCESS;
}

template <typename T_QuantType>
datautil::StatusCode datautil::floatToTfN(
    T_QuantType* out, float* in, int32_t offset, float scale, size_t numElements) {
  static_assert(std::is_unsigned<T_QuantType>::value, "floatToTfN supports unsigned only!");

  if (nullptr == out || nullptr == in) {
    QNN_ERROR("Received a nullptr");
    return StatusCode::INVALID_BUFFER;
  }

  size_t dataTypeSizeInBytes = sizeof(T_QuantType);
  size_t bitWidth            = dataTypeSizeInBytes * g_bitsPerByte;
  double trueBitWidthMax     = pow(2, bitWidth) - 1;
  double encodingMin         = offset * scale;
  double encodingMax         = (trueBitWidthMax + offset) * scale;
  double encodingRange       = encodingMax - encodingMin;

  for (size_t i = 0; i < numElements; ++i) {
    int quantizedValue = round(trueBitWidthMax * (in[i] - encodingMin) / encodingRange);
    //QNN_INFO("Original value: %0.5f. Quantized value: %d", in[i], quantizedValue);
    if (quantizedValue < 0)
      quantizedValue = 0;
    else if (quantizedValue > (int)trueBitWidthMax)
      quantizedValue = (int)trueBitWidthMax;
    out[i] = static_cast<T_QuantType>(quantizedValue);
  }
  return StatusCode::SUCCESS;
}

template datautil::StatusCode datautil::floatToTfN<uint8_t>(
    uint8_t* out, float* in, int32_t offset, float scale, size_t numElements);

template datautil::StatusCode datautil::floatToTfN<uint16_t>(
    uint16_t* out, float* in, int32_t offset, float scale, size_t numElements);

template <typename T_QuantType>
datautil::StatusCode datautil::tfNToFloat(
    float* out, T_QuantType* in, int32_t offset, float scale, size_t numElements) {
  static_assert(std::is_unsigned<T_QuantType>::value, "tfNToFloat supports unsigned only!");

  if (nullptr == out || nullptr == in) {
    QNN_ERROR("Received a nullptr");
    return StatusCode::INVALID_BUFFER;
  }
  for (size_t i = 0; i < numElements; i++) {
    double quantizedValue = static_cast<double>(in[i]);
    double offsetDouble   = static_cast<double>(offset);
    out[i]                = static_cast<double>((quantizedValue + offsetDouble) * scale);
  }
  return StatusCode::SUCCESS;
}

template datautil::StatusCode datautil::tfNToFloat<uint8_t>(
    float* out, uint8_t* in, int32_t offset, float scale, size_t numElements);

template datautil::StatusCode datautil::tfNToFloat<uint16_t>(
    float* out, uint16_t* in, int32_t offset, float scale, size_t numElements);

template <typename T_QuantType>
datautil::StatusCode datautil::castToFloat(float* out, T_QuantType* in, size_t numElements) {
  if (nullptr == out || nullptr == in) {
    QNN_ERROR("Received a nullptr");
    return StatusCode::INVALID_BUFFER;
  }
  for (size_t i = 0; i < numElements; i++) {
    out[i] = static_cast<float>(in[i]);
  }
  return StatusCode::SUCCESS;
}

template datautil::StatusCode datautil::castToFloat<uint8_t>(float* out,
                                                             uint8_t* in,
                                                             size_t numElements);

template datautil::StatusCode datautil::castToFloat<uint16_t>(float* out,
                                                              uint16_t* in,
                                                              size_t numElements);

template datautil::StatusCode datautil::castToFloat<uint32_t>(float* out,
                                                              uint32_t* in,
                                                              size_t numElements);

template datautil::StatusCode datautil::castToFloat<int8_t>(float* out,
                                                            int8_t* in,
                                                            size_t numElements);

template datautil::StatusCode datautil::castToFloat<int16_t>(float* out,
                                                             int16_t* in,
                                                             size_t numElements);

template datautil::StatusCode datautil::castToFloat<int32_t>(float* out,
                                                             int32_t* in,
                                                             size_t numElements);

template <typename T_QuantType>
datautil::StatusCode datautil::castFromFloat(T_QuantType* out, float* in, size_t numElements) {
  if (nullptr == out || nullptr == in) {
    QNN_ERROR("Received a nullptr");
    return StatusCode::INVALID_BUFFER;
  }
  for (size_t i = 0; i < numElements; i++) {
    out[i] = static_cast<T_QuantType>(in[i]);
  }
  return StatusCode::SUCCESS;
}

template datautil::StatusCode datautil::castFromFloat<uint8_t>(uint8_t* out,
                                                               float* in,
                                                               size_t numElements);

template datautil::StatusCode datautil::castFromFloat<uint16_t>(uint16_t* out,
                                                                float* in,
                                                                size_t numElements);

template datautil::StatusCode datautil::castFromFloat<uint32_t>(uint32_t* out,
                                                                float* in,
                                                                size_t numElements);

template datautil::StatusCode datautil::castFromFloat<int8_t>(int8_t* out,
                                                              float* in,
                                                              size_t numElements);

template datautil::StatusCode datautil::castFromFloat<int16_t>(int16_t* out,
                                                               float* in,
                                                               size_t numElements);

template datautil::StatusCode datautil::castFromFloat<int32_t>(int32_t* out,
                                                               float* in,
                                                               size_t numElements);