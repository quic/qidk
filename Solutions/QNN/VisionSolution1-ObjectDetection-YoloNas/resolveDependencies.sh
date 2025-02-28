#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

#RESOLVING DEPENDENCIES

# steps to copy opencv
wget https://sourceforge.net/projects/opencvlibrary/files/4.5.5/opencv-4.5.5-android-sdk.zip/download
unzip download
rm download
mkdir sdk
mv OpenCV-android-sdk/sdk/* sdk
rm -r OpenCV-android-sdk

mkdir -p app/src/main/jniLibs/arm64-v8a
mkdir -p app/src/main/assets


# Check if QNN_SDK_ROOT is set ?
[ -z "$QNN_SDK_ROOT" ] && echo "QNN_SDK_ROOT not set" && exit -1 || echo "QNN_SDK_ROOT Root = ${QNN_SDK_ROOT}"

cp -R $QNN_SDK_ROOT/examples/QNN/SampleApp/src/Log   app/src/main/cpp/
cp -R $QNN_SDK_ROOT/examples/QNN/SampleApp/src/Utils app/src/main/cpp/
cp -R $QNN_SDK_ROOT/examples/QNN/SampleApp/src/PAL   app/src/main/cpp/
cp -R $QNN_SDK_ROOT/include/QNN                      app/src/main/cpp/
cp $QNN_SDK_ROOT/examples/QNN/SampleApp/src/QnnTypeMacros.hpp  app/src/main/cpp/include/
cp $QNN_SDK_ROOT/examples/QNN/SampleApp/src/WrapperUtils/QnnWrapperUtils.hpp  app/src/main/cpp/include/
mv app/src/main/cpp/QNN/*.h app/src/main/cpp/include/
cp $QNN_SDK_ROOT/examples/QNN/SampleApp/src/WrapperUtils/QnnWrapperUtils.cpp  app/src/main/cpp/src/

##writing jniLibs
cp $QNN_SDK_ROOT/lib/hexagon-v73/unsigned/*.so app/src/main/jniLibs/arm64-v8a/
cp $QNN_SDK_ROOT/lib/hexagon-v75/unsigned/*.so app/src/main/jniLibs/arm64-v8a/
cp $QNN_SDK_ROOT/lib/hexagon-v79/unsigned/*.so app/src/main/jniLibs/arm64-v8a/
cp $QNN_SDK_ROOT/lib/aarch64-android/*.so app/src/main/jniLibs/arm64-v8a/
