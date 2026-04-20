#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

cp -r $QNN_SDK_ROOT/include/QNN/ speech_to_image/app/src/main/cpp/speech_to_image/src/
cp -r $QNN_SDK_ROOT/examples/QNN/SampleApp/SampleApp/src/Log speech_to_image/app/src/main/cpp/speech_to_image/src/
cp -r $QNN_SDK_ROOT/examples/QNN/SampleApp/SampleApp/src/PAL speech_to_image/app/src/main/cpp/speech_to_image/src/
cp -r $QNN_SDK_ROOT/examples/QNN/SampleApp/SampleApp/src/WrapperUtils speech_to_image/app/src/main/cpp/speech_to_image/src/
cp -r $QNN_SDK_ROOT/examples/QNN/SampleApp/SampleApp/src/QnnTypeMacros.hpp speech_to_image/app/src/main/cpp/speech_to_image/src/
cp -r $QNN_SDK_ROOT/examples/QNN/SampleApp/SampleApp/src/Utils/BuildId.hpp speech_to_image/app/src/main/cpp/speech_to_image/src/
