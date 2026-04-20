//============================================================================
// Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//============================================================================

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "raw_image_to_png.hpp"
#include "AndroidLogger.hpp"
#include "stb_image_write.h"

bool raw_image_to_png(float *raw, const char *output_file){
  // Convert the RAW data to PNG format
  std::vector<uint8_t> imageData(512 * 512 * 3);

  for (size_t i = 0; i < 512 * 512 * 3; ++i) {
    imageData[i] = static_cast<uint8_t>(std::clamp(raw[i] * 255.0f, 0.0f, 255.0f));
  }

  // Save the image as a PNG file using stb_image_write
  if (!stbi_write_png(output_file, 512, 512, 3, imageData.data(), 512 * 3)) {
    LOGE("Error: Could not save the image!");
    return -1;
  }

  LOGI("Image converted and saved successfully!");
  return true;
}