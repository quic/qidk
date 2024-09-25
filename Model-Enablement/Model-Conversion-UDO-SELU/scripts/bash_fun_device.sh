#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

function sample_fun
{
export SNPE_TARGET_ARCH="aarch64-android-clang8.0"
export SNPE_TARGET_STL="libc++_shared.so"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/local/tmp/snpeexample/$SNPE_TARGET_ARCH/lib
export LD_LIBRARY_PATH=/data/local/tmp/$ONDEVICE_FOLDER/cpu/:$LD_LIBRARY_PATH
export PATH=$PATH:/data/local/tmp/snpeexample/$SNPE_TARGET_ARCH/bin
}
sample_fun
