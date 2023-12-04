# ==============================================================================
#
#  Copyright (c) 2020, 2023 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#
# ===============================================================

LOCAL_PATH := $(call my-dir)
SUPPORTED_TARGET_ABI := arm64-v8a x86 x86_64

#============================ Verify Target Info and Application Variables =========================================
ifneq ($(filter $(TARGET_ARCH_ABI),$(SUPPORTED_TARGET_ABI)),)
ifneq ($(APP_STL), c++_shared)
$(error Unsupported APP_STL: "$(APP_STL)")
endif
else
$(error Unsupported TARGET_ARCH_ABI: '$(TARGET_ARCH_ABI)')
endif

#============================ Define Common Variables ===============================================================
# Include paths
UTIL_SRC_DIR := $(LOCAL_PATH)/../src/utils
# QNN_SDK_ROOT should be set and points to the SDK path, it will be used.
ifdef QNN_SDK_ROOT
ifeq ($(shell test -d ${QNN_SDK_ROOT}/target && echo 0),0)
  # define directories
  CUSTOM_OP_DIR :=$(QNN_SDK_ROOT)/target/x86_64-linux-clang/share/OpPackageGenerator/CustomOp

  # setup include paths
  PACKAGE_C_INCLUDES += -I $(QNN_SDK_ROOT)/include -I $(QNN_SDK_ROOT)/include/CPU -I $(LOCAL_PATH)/../include/ -I $(UTIL_SRC_DIR) -I $(UTIL_SRC_DIR)/CPU -I $(CUSTOM_OP_DIR)
else
  # define directories
  CUSTOM_OP_DIR :=$(QNN_SDK_ROOT)/share/QNN/OpPackageGenerator/CustomOp

  # setup include paths
  PACKAGE_C_INCLUDES += -I $(QNN_SDK_ROOT)/include/QNN -I $(QNN_SDK_ROOT)/include/QNN/CPU -I $(LOCAL_PATH)/../include/ -I $(UTIL_SRC_DIR) -I $(UTIL_SRC_DIR)/CPU -I $(CUSTOM_OP_DIR)
endif
# copy source files from SDK if not present
$(info Copying custom op source files from SDK)
COPYFILES := $(shell find $(CUSTOM_OP_DIR)/CPU -name "*.cpp" -exec cp -rf {} $(LOCAL_PATH)/../src 2>/dev/null \;)
else
$(error QNN_SDK_ROOT: Please set QNN_SDK_ROOT)
endif

#========================== Define OpPackage Library Build Variables =============================================
include $(CLEAR_VARS)
LOCAL_C_INCLUDES               := $(PACKAGE_C_INCLUDES)
MY_SRC_FILES                    = $(wildcard $(LOCAL_PATH)/../src/*.cpp) $(wildcard $(LOCAL_PATH)/../src/utils/*.cpp) $(wildcard $(LOCAL_PATH)/../src/utils/CPU/*.cpp) $(wildcard $(LOCAL_PATH)/../src/ops/*.cpp)
LOCAL_MODULE                   := RandomNormalLike
LOCAL_SRC_FILES                := $(subst makefiles/,,$(MY_SRC_FILES))
LOCAL_LDLIBS                   := -lGLESv2 -lEGL
include $(BUILD_SHARED_LIBRARY)
