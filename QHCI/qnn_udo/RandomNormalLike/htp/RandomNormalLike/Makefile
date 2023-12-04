#=============================================================================
#  Copyright (c) 2020-2021, 2023 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#=============================================================================

# users should provide locations for QNN_INCLUDE and HEXAGON_SDK
# export HEXAGON_SDK_ROOT = /path/to/hexagon-sdk

# check all setup prerequisites if the command goal is not clean
ifneq ($(MAKECMDGOALS),clean)
ifeq ($(shell test -d ${QNN_SDK_ROOT}/target && echo 0),0)
ifndef QNN_INCLUDE
$(info "INFO: Qnn include not explicitly defined, attempting to use QNN_SDK_ROOT if it is valid")
QNN_INCLUDE := $(QNN_SDK_ROOT)/include
endif
ifeq ($(wildcard $(QNN_INCLUDE)),)
$(error "ERROR: QNN_INCLUDE path is not set. QNN include paths must be set to obtain BE headers necessary to compile the package")
endif
ifndef QNN_TARGET_LIB
$(info "INFO: Qnn target not explicitly defined, attempting to use QNN_SDK_ROOT if it is valid")
QNN_TARGET_LIB := $(QNN_SDK_ROOT)/target/aarch64-android/lib
endif
else
ifndef QNN_INCLUDE
$(info "INFO: Qnn include not explicitly defined, attempting to use QNN_SDK_ROOT if it is valid")
QNN_INCLUDE := $(QNN_SDK_ROOT)/include/QNN
endif
ifeq ($(wildcard $(QNN_INCLUDE)),)
$(error "ERROR: QNN_INCLUDE path is not set. QNN include paths must be set to obtain BE headers necessary to compile the package")
endif
ifndef QNN_TARGET_LIB
$(info "INFO: Qnn target not explicitly defined, attempting to use QNN_SDK_ROOT if it is valid")
QNN_TARGET_LIB := $(QNN_SDK_ROOT)/lib/aarch64-android
endif
endif
ifeq ($(wildcard $(QNN_TARGET_LIB)),)
ifeq ($(MAKECMDGOALS),htp_aarch64)
$(error "ERROR: QNN_TARGET_LIB is needed to compile package for aarch64")
else ifeq ($(MAKECMDGOALS),all)
$(info "WARNING:QNN_TARGET_LIB may need to be defined to compile packages")
endif
endif

ifndef HEXAGON_SDK_ROOT
# depending on setup instructions this variable may have a different name
ifdef HEXAGON_SDK
HEXAGON_SDK_ROOT := $(HEXAGON_SDK)
else
$(error "ERROR: HEXAGON_SDK_ROOT is not set. Hexagon-SDK path must be set to the latest hexagon-sdk-x.y.z")
endif
endif

ifeq ($(wildcard $(HEXAGON_SDK_ROOT)),)
$(error "ERROR: HEXAGON_SDK_ROOT or HEXAGON_SDK path is not set correctly. Please set HEXAGON_SDK to latest hexagon-sdk-X.Y.Z path")
endif

HEXAGON_SDK_BASE := $(dir $(HEXAGON_SDK_ROOT))

$(info "HEXAGON_SDK_ROOT is [${HEXAGON_SDK_ROOT}]")
# Users should note that the tools version may change between hexagon sdk versions
HEXAGON_TOOLS_VERSION_X86 := 8.6.0
HEXAGON_TOOLS_VERSION_V68 := 8.6.0
HEXAGON_TOOLS_VERSION_V69 := 8.5.03
HEXAGON_TOOLS_VERSION_V73 := 8.6.02
$(info "INFO: HEXAGON_TOOLS_VERSION is not set. Using default versions.")
else
HEXAGON_TOOLS_VERSION_X86 := $(HEXAGON_TOOLS_VERSION)
HEXAGON_TOOLS_VERSION_V68 := $(HEXAGON_TOOLS_VERSION)
HEXAGON_TOOLS_VERSION_V69 := $(HEXAGON_TOOLS_VERSION)
HEXAGON_TOOLS_VERSION_V73 := $(HEXAGON_TOOLS_VERSION)
$(info "INFO: HEXAGON_TOOLS_VERSION is set to $(HEXAGON_TOOLS_VERSION).")
endif

ifndef ANDROID_NDK_ROOT
ifeq ($(MAKECMDGOALS),htp_aarch64)
$(error "ERROR: ANDROID_NDK_ROOT is not set. Android NDK path must be set to compile package for aarch64")
else ifeq ($(MAKECMDGOALS),all)
$(info "WARNING: ANDROID_NDK_ROOT is not set. Android NDK path must be set to compile package for aarch64")
endif
endif

ifndef PACKAGE_NAME
export
PACKAGE_NAME := $(notdir $(shell pwd))
$(info "INFO: No package name defined. Using current directory name: $(PACKAGE_NAME) as the package name")
endif

WORK := build
SRC_DIR := src
OP_SRC_DIR := src/ops
OP_INCLUDE_DIR := ./include
OP_INCLUDES =  #$(wildcard $(OP_INCLUDE_DIR)/*.h) user defined if any op specific headers are needed, add -I to common flags
LIBRARY_NAME := libQnn$(PACKAGE_NAME).so
SUPPORTED_TARGETS = x86_64-linux-clang hexagon-v68 hexagon-v69 hexagon-v73 aarch64-android


COMMON_CXX_FLAGS = -std=c++17 -I$(QNN_INCLUDE) -fPIC -Wall -Wreorder -Wno-missing-braces
COMMON_CXX_FLAGS += -Werror -Wno-format -Wno-unused-command-line-argument -fvisibility=default -stdlib=libc++
COMMON_CXX_FLAGS += -DQNN_API="__attribute__((visibility(\"default\")))"  -D__QAIC_HEADER_EXPORT="__attribute__((visibility(\"default\")))"

X86_LIBNATIVE_RELEASE_DIR := $(HEXAGON_SDK_ROOT)/tools/HEXAGON_Tools/$(HEXAGON_TOOLS_VERSION_X86)/Tools

# Ensure hexagon sdk tool version can be retrieved
ifeq ($(wildcard $(X86_LIBNATIVE_RELEASE_DIR)/.),)
$(error "Cannot retrieve hexagon tools from: $(X86_LIBNATIVE_RELEASE_DIR).  \
         \
         Please check that hexagon tools version is correct. Expected: $(HEXAGON_TOOLS_VERSION_X86)")
endif

OP_SOURCES = $(wildcard $(OP_SRC_DIR)/*.cpp)
OTHER_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
HFILES = $(wildcard $(QNN_INCLUDE)/*.h)
HFILES += $(wildcard $(QNN_INCLUDE)/HTP/*.h)
HFILES += $(wildcard $(QNN_INCLUDE)/HTP/core/*.h)
OP_OBJS = $(patsubst $(SRC_DIR)/%,%,$(patsubst %.cpp,%.o,$(OP_SOURCES)))
OTHER_OBJS =  $(patsubst $(SRC_DIR)/%,%,$(patsubst %.cpp,%.o,$(OTHER_SOURCES)))


all: htp_v68 htp_x86 htp_aarch64

#============================================================================================================
# Setup compiler, compiler instructions and linker for x86
X86_CXX ?= clang++
X86_LDFLAGS:= -Wl,--whole-archive -L$(X86_LIBNATIVE_RELEASE_DIR)/libnative/lib -lnative -Wl,--no-whole-archive -lpthread
X86_C_FLAGS := -D__HVXDBL__ -I$(X86_LIBNATIVE_RELEASE_DIR)/libnative/include -ffast-math -DUSE_OS_LINUX
X86_CXX_FLAGS = $(COMMON_CXX_FLAGS) $(X86_C_FLAGS) -fomit-frame-pointer -Wno-invalid-offsetof
linux_objs =
#============================================================================================================
# Setup compiler, compiler instructions and linker for hexagon
HEXAGON_CXX_FLAGS := $(COMMON_CXX_FLAGS) -mhvx -mhvx-length=128B -mhmx -DUSE_OS_QURT -O2 -Wno-reorder
HEXAGON_CXX_FLAGS += -I$(HEXAGON_SDK_ROOT)/incs -I$(HEXAGON_SDK_ROOT)/incs/stddef

HEXAGON_CXX_FLAGS_V68 := $(HEXAGON_CXX_FLAGS) -mv68 -I$(HEXAGON_SDK_ROOT)/rtos/qurt/computev68/include/qurt -I$(HEXAGON_SDK_ROOT)/rtos/qurt/computev68/include/posix
HEXAGON_CXX_FLAGS_V69 := $(HEXAGON_CXX_FLAGS) -mv69 -I$(HEXAGON_SDK_ROOT)/rtos/qurt/computev69/include/qurt -I$(HEXAGON_SDK_ROOT)/rtos/qurt/computev69/include/posix
HEXAGON_CXX_FLAGS_V73 := $(HEXAGON_CXX_FLAGS) -mv73 -I$(HEXAGON_SDK_ROOT)/rtos/qurt/computev73/include/qurt -I$(HEXAGON_SDK_ROOT)/rtos/qurt/computev73/include/posix

HEXAGON_CXX_V68 := $(HEXAGON_SDK_ROOT)/tools/HEXAGON_Tools/$(HEXAGON_TOOLS_VERSION_V68)/Tools/bin/hexagon-clang++
HEXAGON_CXX_V69 := $(HEXAGON_SDK_ROOT)/tools/HEXAGON_Tools/$(HEXAGON_TOOLS_VERSION_V69)/Tools/bin/hexagon-clang++
HEXAGON_CXX_V73 := $(HEXAGON_SDK_ROOT)/tools/HEXAGON_Tools/$(HEXAGON_TOOLS_VERSION_V73)/Tools/bin/hexagon-clang++

HEX_LDFLAGS =
hexagon_objs =
#============================================================================================================
# Setup compiler, compiler instructions and linker for aarch64
AARCH64_C__FLAGS = -D__HVXDBL__ -I$(X86_LIBNATIVE_RELEASE_DIR)/libnative/include -ffast-math -DUSE_OS_LINUX -DANDROID
AARCH64_CXX_FLAGS = $(COMMON_CXX_FLAGS) $(AARCH64_C__FLAGS) -fomit-frame-pointer -Wno-invalid-offsetof  -Wno-unused-variable -Wno-unused-parameter -Wno-missing-braces -Wno-sign-compare -Wno-unused-private-field -Wno-unused-variable -Wno-ignored-qualifiers -Wno-missing-field-initializers
ARM_CLANG_OPTS =--target=aarch64-none-linux-android21 --sysroot=$(ANDROID_NDK_ROOT)/toolchains/llvm/prebuilt/linux-x86_64/sysroot
AARCH64_CXX = $(ANDROID_NDK_ROOT)/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++ $(ARM_CLANG_OPTS)
AARCH64_LDFLAGS = -L$(QNN_TARGET)/aarch64-android/lib -lQnnHtp -lQnnHtpPrepare
aarch64_objs =
#============================================================================================================
# Setup targets and goals

htp_x86: X86_BUILD

htp_v68: HEXAGON_BUILD_V68

htp_v69: HEXAGON_BUILD_V69

htp_v73: HEXAGON_BUILD_V73

htp_aarch64: AARCH64_BUILD

AARCH64_BUILD: $(WORK)/aarch64-android/$(LIBRARY_NAME)

HEXAGON_BUILD_V68:  $(WORK)/hexagon-v68/$(LIBRARY_NAME)

HEXAGON_BUILD_V69:  $(WORK)/hexagon-v69/$(LIBRARY_NAME)

HEXAGON_BUILD_V73:  $(WORK)/hexagon-v73/$(LIBRARY_NAME)

X86_BUILD: $(WORK)/x86_64-linux-clang/$(LIBRARY_NAME)


define build_objs =
ifneq ($(filter $(2),$(SUPPORTED_TARGETS)),)
$(2)_objs += $(foreach x,$(1),$(WORK)/$(2)/$(x))
else
$$(error "Unknown target option provided: $(2): Supported targets are: $(SUPPORTED_TARGETS)")
endif
endef

$(eval $(call build_objs,$(OTHER_OBJS),x86_64-linux-clang))
$(eval $(call build_objs,$(OP_OBJS),x86_64-linux-clang))
$(eval $(call build_objs,$(OTHER_OBJS),hexagon-v68))
$(eval $(call build_objs,$(OP_OBJS),hexagon-v68))
$(eval $(call build_objs,$(OTHER_OBJS),hexagon-v69))
$(eval $(call build_objs,$(OP_OBJS),hexagon-v69))
$(eval $(call build_objs,$(OTHER_OBJS),hexagon-v73))
$(eval $(call build_objs,$(OP_OBJS),hexagon-v73))
$(eval $(call build_objs,$(OTHER_OBJS),aarch64-android))
$(eval $(call build_objs,$(OP_OBJS),aarch64-android))

# x86
$(WORK)/x86_64-linux-clang $(WORK)/hexagon-v68 $(WORK)/hexagon-v69 $(WORK)/hexagon-v73 $(WORK)/aarch64-android:
	@mkdir -p $@/ops

$(WORK)/x86_64-linux-clang/%.o: $(SRC_DIR)/%.cpp | $(WORK)/x86_64-linux-clang
	$(X86_CXX) $(X86_CXX_FLAGS) -DTHIS_PKG_NAME=$(PACKAGE_NAME) -MMD -c $< -o $@

$(WORK)/x86_64-linux-clang/ops/%.o: $(OP_SRC_DIR)/%.cpp | $(WORK)/x86_64-linux-clang
	$(X86_CXX) $(X86_CXX_FLAGS) -DTHIS_PKG_NAME=$(PACKAGE_NAME) -MMD -c $< -o $@

$(WORK)/x86_64-linux-clang/$(LIBRARY_NAME): $(x86_64-linux-clang_objs) | $(HFILES)
	$(X86_CXX) -fPIC -std=c++17 -g -shared -o $@ $^ $(X86_LDFLAGS)

# v68
$(WORK)/hexagon-v68/%.o: $(SRC_DIR)/%.cpp | $(WORK)/hexagon-v68
	$(HEXAGON_CXX_V68) $(HEXAGON_CXX_FLAGS_V68) -DTHIS_PKG_NAME=$(PACKAGE_NAME) -MMD -c $< -o $@

$(WORK)/hexagon-v68/ops/%.o: $(OP_SRC_DIR)/%.cpp | $(WORK)/hexagon-v68
	$(HEXAGON_CXX_V68) $(HEXAGON_CXX_FLAGS_V68) -DTHIS_PKG_NAME=$(PACKAGE_NAME) -MMD -c $< -o $@

$(WORK)/hexagon-v68/$(LIBRARY_NAME): $(hexagon-v68_objs) | $(HFILES)
	$(HEXAGON_CXX_V68) -fPIC -std=c++17 -g -shared -o $@ $^ $(HEX_LDFLAGS)

# v69
$(WORK)/hexagon-v69/%.o: $(SRC_DIR)/%.cpp | $(WORK)/hexagon-v69
	$(HEXAGON_CXX_V69) $(HEXAGON_CXX_FLAGS_V69) -DTHIS_PKG_NAME=$(PACKAGE_NAME) -MMD -c $< -o $@

$(WORK)/hexagon-v69/ops/%.o: $(OP_SRC_DIR)/%.cpp | $(WORK)/hexagon-v69
	$(HEXAGON_CXX_V69) $(HEXAGON_CXX_FLAGS_V69) -DTHIS_PKG_NAME=$(PACKAGE_NAME) -MMD -c $< -o $@

$(WORK)/hexagon-v69/$(LIBRARY_NAME): $(hexagon-v69_objs) | $(HFILES)
	$(HEXAGON_CXX_V69) -fPIC -std=c++17 -g -shared -o $@ $^ $(HEX_LDFLAGS)

# v73
$(WORK)/hexagon-v73/%.o: $(SRC_DIR)/%.cpp | $(WORK)/hexagon-v73
	$(HEXAGON_CXX_V73) $(HEXAGON_CXX_FLAGS_V73) -DTHIS_PKG_NAME=$(PACKAGE_NAME) -MMD -c $< -o $@

$(WORK)/hexagon-v73/ops/%.o: $(OP_SRC_DIR)/%.cpp | $(WORK)/hexagon-v73
	$(HEXAGON_CXX_V73) $(HEXAGON_CXX_FLAGS_V73) -DTHIS_PKG_NAME=$(PACKAGE_NAME) -MMD -c $< -o $@

$(WORK)/hexagon-v73/$(LIBRARY_NAME): $(hexagon-v73_objs) | $(HFILES)
	$(HEXAGON_CXX_V73) -fPIC -std=c++17 -g -shared -o $@ $^ $(HEX_LDFLAGS)

# aarch64
$(WORK)/aarch64-android/%.o: $(SRC_DIR)/%.cpp | $(WORK)/aarch64-android
	$(AARCH64_CXX) $(AARCH64_CXX_FLAGS) -DTHIS_PKG_NAME=$(PACKAGE_NAME) -MMD -c $< -o $@

$(WORK)/aarch64-android/ops/%.o: $(OP_SRC_DIR)/%.cpp | $(WORK)/aarch64-android
	$(AARCH64_CXX) $(AARCH64_CXX_FLAGS) -DTHIS_PKG_NAME=$(PACKAGE_NAME) -MMD -c $< -o $@

$(WORK)/aarch64-android/$(LIBRARY_NAME): $(aarch64-android_objs) | $(HFILES)
	$(AARCH64_CXX) -fPIC -std=c++17 -g -shared -o $@ $^ $(AARCH64_LDFLAGS)

clean:
	-rm -rf $(WORK)

.PHONY: all clean
