//=============================================================================
//
//  Copyright (c) 2020-2022 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//=============================================================================

#include "CPU/QnnCpuOpPackage.h"
#include "CustomBEMacros.hpp"
#include "CustomOpPackage.hpp"
#include "QnnSdkBuildId.h"

using namespace qnn::custom;
using namespace qnn::custom::utils;

static Qnn_ErrorHandle_t QnnOpPackage_execute(void* opPkgNodeData) {
  auto opPkg = CustomOpPackage::getInstance();
  std::shared_ptr<CustomOp> op;

  opPkg->getOpResolver()->getCustomOp((opHandle)opPkgNodeData, op);
  auto opRegistration = opPkg->getOpRegistration(op->m_typeName);

  QNN_CUSTOM_BE_ENSURE(opPkg, QNN_OP_PACKAGE_ERROR_GENERAL);
  QNN_CUSTOM_BE_ENSURE_STATUS(opRegistration->execute(op.get()));

  return QNN_SUCCESS;
}

std::mutex CustomOpPackage::s_mtx;
std::shared_ptr<CustomOpPackage> CustomOpPackage ::s_opPackageInstance;
bool CustomOpPackage::s_isInitialized;

Qnn_ErrorHandle_t CustomOpPackage::getPackageInfo(const QnnOpPackage_Info_t** info) {
  QNN_CUSTOM_BE_ENSURE(info, QNN_OP_PACKAGE_ERROR_INVALID_INFO)

  for (auto op : m_registered_ops) {
    m_operationNames.push_back(op.first.c_str());
  }

  m_sdkApiVersion              = QNN_CPU_API_VERSION_INIT;
  m_packageInfo                = QNN_OP_PACKAGE_INFO_INIT;
  m_packageInfo.packageName    = m_packageName;
  m_packageInfo.operationNames = m_operationNames.data();
  m_packageInfo.numOperations  = static_cast<uint32_t>(m_operationNames.size());
  m_packageInfo.sdkBuildId     = QNN_SDK_BUILD_ID;
  m_packageInfo.sdkApiVersion  = &m_sdkApiVersion;
  *info                        = &m_packageInfo;

  return QNN_SUCCESS;
}

Qnn_ErrorHandle_t CustomOpPackage::createOpImpl(
    QnnOpPackage_GraphInfrastructure_t graphInfrastructure,
    QnnOpPackage_Node_t node,
    QnnOpPackage_OpImpl_t* opImplPtr) {
  // initialize op resolver if not already set
  if (!m_opResolver) {
    m_opResolver.reset(new CustomOpResolver());
  }
  auto cpuNode  = reinterpret_cast<QnnCpuOpPackage_Node_t*>(node);
  auto customOp = std::shared_ptr<CustomOp>(new CustomOp(cpuNode->name, cpuNode->typeName));
  const auto opRegistration = m_registered_ops[cpuNode->typeName];

  // Get op from op factory
  QNN_CUSTOM_BE_ENSURE_STATUS(
      opRegistration->initialize(node, graphInfrastructure, customOp.get()));

  // Update op reference
  auto opImpl      = std::make_shared<QnnCpuOpPackage_OpImpl_t>();
  opImpl->opImplFn = QnnOpPackage_execute;
  opImpl->userData = (void*)m_opResolver->registerCustomOp(std::move(customOp));

  // update out kernel param
  auto cpuImpl = reinterpret_cast<QnnCpuOpPackage_OpImpl_t**>(opImplPtr);
  *cpuImpl     = opImpl.get();

  // update opImpl list
  m_OpImplList.emplace_back(opImpl);

  return QNN_SUCCESS;
}

Qnn_ErrorHandle_t CustomOpPackage::freeOpImpl(QnnOpPackage_OpImpl_t opImpl) {
  QNN_CUSTOM_BE_ENSURE(opImpl, QNN_OP_PACKAGE_ERROR_GENERAL);

  auto op = std::shared_ptr<CustomOp>(new CustomOp());

  auto cpuOpImpl = reinterpret_cast<QnnCpuOpPackage_OpImpl_t*>(opImpl);
  m_opResolver->getCustomOp((opHandle)cpuOpImpl->userData, op);

  auto opRegistration = m_registered_ops[op->m_typeName];
  QNN_CUSTOM_BE_ENSURE_STATUS(m_opResolver->removeCustomOp((opHandle)cpuOpImpl->userData));

  if (opRegistration->free) {
    opRegistration->free(*op);
  }

  return QNN_SUCCESS;
}

std::shared_ptr<CustomOpPackage> CustomOpPackage::getInstance() noexcept {
  std::lock_guard<std::mutex> locker(s_mtx);
  if (!s_opPackageInstance) {
    s_opPackageInstance.reset(new (std::nothrow) CustomOpPackage());
  }
  return s_opPackageInstance;
}

void CustomOpPackage::setIsInitialized(bool isInitialized) {
  std::lock_guard<std::mutex> locker(s_mtx);
  s_isInitialized = isInitialized;
}

bool CustomOpPackage::getIsInitialized() {
  std::lock_guard<std::mutex> locker(s_mtx);
  return s_isInitialized;
}

void CustomOpPackage::destroyInstance() {
  if (s_opPackageInstance && s_isInitialized) s_opPackageInstance.reset();
  s_isInitialized = false;
}

void CustomOpPackage::freeResolver() {
  if (m_opResolver) m_opResolver.reset();
}
