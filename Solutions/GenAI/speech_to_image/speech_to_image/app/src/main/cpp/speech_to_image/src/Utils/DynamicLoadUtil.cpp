//==============================================================================
//
//  Copyright (c) 2025 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================


#include <iostream>

#include "DynamicLoadUtil.hpp"
#include "Logger.hpp"
#include "PAL/DynamicLoading.hpp"

using namespace qnn;
using namespace qnn::tools;

typedef Qnn_ErrorHandle_t (*QnnInterfaceGetProvidersFn_t)(const QnnInterface_t*** providerList,
                                                          uint32_t* numProviders);

typedef Qnn_ErrorHandle_t (*QnnSystemInterfaceGetProvidersFn_t)(
    const QnnSystemInterface_t*** providerList, uint32_t* numProviders);

template <class T>
static inline T resolveSymbol(void* libHandle, const char* sym) {
  T ptr = (T)pal::dynamicloading::dlSym(libHandle, sym);
  if (ptr == nullptr) {
    QNN_ERROR("Unable to access symbol [%s]. pal::dynamicloading::dlError(): %s",
              sym,
              pal::dynamicloading::dlError());
  }
  return ptr;
}

dynamicloadutil::StatusCode dynamicloadutil::getQnnFunctionPointers(
    std::string backendPath,
    speech_to_image::QnnFunctionPointers* qnnFunctionPointers,
    void** backendHandleRtn) {
  void* libBackendHandle = pal::dynamicloading::dlOpen(
      backendPath.c_str(), pal::dynamicloading::DL_NOW | pal::dynamicloading::DL_LOCAL);
  if (nullptr == libBackendHandle) {
    QNN_ERROR("Unable to load backend. pal::dynamicloading::dlError(): %s",
              pal::dynamicloading::dlError());
    return StatusCode::FAIL_LOAD_BACKEND;
  }
  if (nullptr != backendHandleRtn) {
    *backendHandleRtn = libBackendHandle;
  }
  // Get QNN Interface
  QnnInterfaceGetProvidersFn_t getInterfaceProviders{nullptr};
  getInterfaceProviders =
      resolveSymbol<QnnInterfaceGetProvidersFn_t>(libBackendHandle, "QnnInterface_getProviders");
  if (nullptr == getInterfaceProviders) {
    return StatusCode::FAIL_SYM_FUNCTION;
  }
  QnnInterface_t** interfaceProviders{nullptr};
  uint32_t numProviders{0};
  if (QNN_SUCCESS !=
      getInterfaceProviders((const QnnInterface_t***)&interfaceProviders, &numProviders)) {
    QNN_ERROR("Failed to get interface providers.");
    return StatusCode::FAIL_GET_INTERFACE_PROVIDERS;
  }
  if (nullptr == interfaceProviders) {
    QNN_ERROR("Failed to get interface providers: null interface providers received.");
    return StatusCode::FAIL_GET_INTERFACE_PROVIDERS;
  }
  if (0 == numProviders) {
    QNN_ERROR("Failed to get interface providers: 0 interface providers.");
    return StatusCode::FAIL_GET_INTERFACE_PROVIDERS;
  }
  bool foundValidInterface{false};
  for (size_t pIdx = 0; pIdx < numProviders; pIdx++) {
    if (QNN_API_VERSION_MAJOR == interfaceProviders[pIdx]->apiVersion.coreApiVersion.major &&
        QNN_API_VERSION_MINOR <= interfaceProviders[pIdx]->apiVersion.coreApiVersion.minor) {
      foundValidInterface               = true;
      qnnFunctionPointers->qnnInterface = interfaceProviders[pIdx]->QNN_INTERFACE_VER_NAME;
      break;
    }
  }
  if (!foundValidInterface) {
    QNN_ERROR("Unable to find a valid interface.");
    libBackendHandle = nullptr;
    return StatusCode::FAIL_GET_INTERFACE_PROVIDERS;
  }
  return StatusCode::SUCCESS;
}

dynamicloadutil::StatusCode dynamicloadutil::getQnnSystemFunctionPointers(
    std::string systemLibraryPath, speech_to_image::QnnFunctionPointers* qnnFunctionPointers) {
  QNN_FUNCTION_ENTRY_LOG;
  if (!qnnFunctionPointers) {
    QNN_ERROR("nullptr provided for qnnFunctionPointers");
    return StatusCode::FAILURE;
  }
  void* systemLibraryHandle = pal::dynamicloading::dlOpen(
      systemLibraryPath.c_str(), pal::dynamicloading::DL_NOW | pal::dynamicloading::DL_LOCAL);
  if (nullptr == systemLibraryHandle) {
    QNN_ERROR("Unable to load system library. pal::dynamicloading::dlError(): %s",
              pal::dynamicloading::dlError());
    return StatusCode::FAIL_LOAD_SYSTEM_LIB;
  }
  QnnSystemInterfaceGetProvidersFn_t getSystemInterfaceProviders{nullptr};
  getSystemInterfaceProviders = resolveSymbol<QnnSystemInterfaceGetProvidersFn_t>(
      systemLibraryHandle, "QnnSystemInterface_getProviders");
  if (nullptr == getSystemInterfaceProviders) {
    return StatusCode::FAIL_SYM_FUNCTION;
  }
  QnnSystemInterface_t** systemInterfaceProviders{nullptr};
  uint32_t numProviders{0};
  if (QNN_SUCCESS != getSystemInterfaceProviders(
                         (const QnnSystemInterface_t***)&systemInterfaceProviders, &numProviders)) {
    QNN_ERROR("Failed to get system interface providers.");
    return StatusCode::FAIL_GET_INTERFACE_PROVIDERS;
  }
  if (nullptr == systemInterfaceProviders) {
    QNN_ERROR("Failed to get system interface providers: null interface providers received.");
    return StatusCode::FAIL_GET_INTERFACE_PROVIDERS;
  }
  if (0 == numProviders) {
    QNN_ERROR("Failed to get interface providers-: 0 interface providers.");
    return StatusCode::FAIL_GET_INTERFACE_PROVIDERS;
  }
  bool foundValidSystemInterface{false};
  for (size_t pIdx = 0; pIdx < numProviders; pIdx++) {
    if (QNN_SYSTEM_API_VERSION_MAJOR == systemInterfaceProviders[pIdx]->systemApiVersion.major &&
        QNN_SYSTEM_API_VERSION_MINOR <= systemInterfaceProviders[pIdx]->systemApiVersion.minor) {
      foundValidSystemInterface = true;
      qnnFunctionPointers->qnnSystemInterface =
          systemInterfaceProviders[pIdx]->QNN_SYSTEM_INTERFACE_VER_NAME;
      break;
    }
  }
  if (!foundValidSystemInterface) {
    QNN_ERROR("Unable to find a valid system interface.");
    return StatusCode::FAIL_GET_INTERFACE_PROVIDERS;
  }
  QNN_FUNCTION_EXIT_LOG;
  return StatusCode::SUCCESS;
}