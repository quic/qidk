# Copyright (c) 2020 - 2021, 2023 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.

# define default
default: all

# define package name
export PACKAGE_NAME := $(notdir $(shell pwd))

# define library prerequisites list
lib_cpu := src
make_dir := makefiles
LIB_SOURCES = $(lib_cpu))

# define target_architecture
export TARGET_AARCH_VARS:= -march=x86-64

# define target name
export TARGET = linux-x86_64

# specify compiler
export CXX ?= clang++-9

# define default Android ABI
PLATFORM ?= arm64-v8a

.PHONY: all $(LIB_SOURCES) all_android all_x86 cpu cpu_x86 cpu_android
all: $(LIB_SOURCES) all_x86 all_android

# Combined Targets
cpu: cpu_x86 cpu_android
clean: clean_x86 clean_android clean_qnx

# x86 Targets
all_x86: cpu_x86

cpu_x86:
	$(call build_if_exists,$(lib_cpu),-$(MAKE) -f $(make_dir)/Makefile.linux-x86_64)

clean_x86:
	@rm -rf libs obj

# qnx Targets
all_qnx: cpu_qnx

cpu_qnx: check_qnx
	$(call build_if_exists,$(lib_cpu),-$(MAKE) -f $(make_dir)/Makefile.qnx-aarch64)

clean_qnx:
	@rm -rf libs obj

# Android Targets

all_android: cpu_android

cpu_android: cpu_aarch64-android

cpu_aarch64-android: check_ndk clean_aarch64-android
	$(call build_if_exists,$(lib_cpu),$(ANDROID_NDK_ROOT)/ndk-build APP_ALLOW_MISSING_DEPS=true APP_ABI="arm64-v8a" NDK_PROJECT_PATH=./ NDK_APPLICATION_MK=$(make_dir)/Application.mk APP_BUILD_SCRIPT=$(make_dir)/Android.mk)
	@$(rename_target_dirs)

clean_android: check_ndk clean_aarch64-android

clean_aarch64-android:
	@rm -rf libs/aarch64-android
	@rm -rf obj/local/aarch64-android

# utilities
# Syntax: $(call build_if_exists <dir>,<cmd>)
build_if_exists = $(if $(wildcard $(1)),$(2),$(warning WARNING: $(1) does not exist. Skipping Compilation))
rename_target_dirs = find . -type d -execdir rename 's/arm64-v8a/aarch64-android/' '{}' \+ \

check_ndk:
ifeq ($(ANDROID_NDK_ROOT),)
	$(error ERROR: ANDROID_NDK_ROOT not set, skipping compilation for Android platform(s).)
endif

check_qnx:
ifeq ($(QNX_HOST),)
	$(error ERROR: QNX_HOST not set, skipping compilation for QNX platform.)
endif
ifeq ($(QNX_TARGET),)
	$(error ERROR: QNX_TARGET not set, skipping compilation for QNX platform.)
endif