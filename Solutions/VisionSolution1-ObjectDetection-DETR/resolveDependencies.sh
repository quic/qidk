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


# Check if SNPE_ROOT is set ?
[ -z "$SNPE_ROOT" ] && echo "SNPE_ROOT not set" && exit -1 || echo "SNPE Root = ${SNPE_ROOT}"

#Copy SNPE header files to App CPP include directory
mkdir -p ./app/src/main/cpp/inc/zdl/
cp -R $SNPE_ROOT/include/SNPE/* ./app/src/main/cpp/inc/zdl/

#Steps to paste files in JNI
##copying snpe-release.aar file
mkdir snpe-release
cp $SNPE_ROOT/lib/android/snpe-release.aar snpe-release
unzip -o snpe-release/snpe-release.aar -d snpe-release/snpe-release

mkdir -p app/src/main/jniLibs/arm64-v8a

##writing jniLibs
cp snpe-release/snpe-release/jni/arm64-v8a/libSNPE.so app/src/main/jniLibs/arm64-v8a/
cp snpe-release/snpe-release/jni/arm64-v8a/libsnpe-android.so app/src/main/jniLibs/arm64-v8a/
cp snpe-release/snpe-release/jni/arm64-v8a/libSnpeHtpPrepare.so app/src/main/jniLibs/arm64-v8a/
cp snpe-release/snpe-release/jni/arm64-v8a/libSnpeHtpV73Skel.so app/src/main/jniLibs/arm64-v8a/
cp snpe-release/snpe-release/jni/arm64-v8a/libSnpeHtpV73Stub.so app/src/main/jniLibs/arm64-v8a/
cp snpe-release/snpe-release/jni/arm64-v8a/libSnpeHtpV75Skel.so app/src/main/jniLibs/arm64-v8a/
cp snpe-release/snpe-release/jni/arm64-v8a/libSnpeHtpV75Stub.so app/src/main/jniLibs/arm64-v8a/
cp snpe-release/snpe-release/jni/arm64-v8a/libSnpeHtpV79Skel.so app/src/main/jniLibs/arm64-v8a/
cp snpe-release/snpe-release/jni/arm64-v8a/libSnpeHtpV79Stub.so app/src/main/jniLibs/arm64-v8a/
