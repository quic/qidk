#============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#============================================================================

# Check if SNPE_ROOT is set ?
[ -z "$SNPE_ROOT" ] && echo "SNPE_ROOT not set" && exit -1 || echo "SNPE Root = ${SNPE_ROOT}"


#Copy SNPE header to App CPP include directory
mkdir -p ./SentimentAnalysis/app/src/main/cpp/inc/zdl/
cp -R $SNPE_ROOT/include/SNPE/* ./SentimentAnalysis/app/src/main/cpp/inc/zdl/
 
# Add DLC to App asssets dir
cp ./frozen_models/mobilebert_sst2_cached.dlc ./SentimentAnalysis/app/src/main/assets/
# Validate if DLC is copied
[ -f "./SentimentAnalysis/app/src/main/assets/mobilebert_sst2_cached.dlc" ] && echo "True" || echo "DLC not found in assets dir"

# Add SNPE ARM64 libs to App 'cmakeLibs' and 'jniLibs'
mkdir -p ./SentimentAnalysis/app/src/main/cmakeLibs/arm64-v8a
mkdir -p ./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a
cp $SNPE_ROOT/lib/aarch64-android/libSNPE.so ./SentimentAnalysis/app/src/main/cmakeLibs/arm64-v8a/.
cp $SNPE_ROOT/lib/aarch64-android/libSNPE.so ./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/.
cp $SNPE_ROOT/lib/aarch64-android/libSnpeHtpPrepare.so ./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/.
cp $SNPE_ROOT/lib/aarch64-android/libSnpeHtpV69Stub.so ./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/.
cp $SNPE_ROOT/lib/aarch64-android/libSnpeHtpV73Stub.so ./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/.
cp $SNPE_ROOT/lib/aarch64-android/libSnpeHtpV75Stub.so  ./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/.
cp $SNPE_ROOT/lib/aarch64-android/libSnpeHtpV79Stub.so  ./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/.

# Add SNPE DSP libs to App 'jniLibs'
cp $SNPE_ROOT/lib/hexagon-v69/unsigned/libSnpeHtpV69Skel.so ./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/.
cp $SNPE_ROOT/lib/hexagon-v73/unsigned/libSnpeHtpV73Skel.so ./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/.
cp $SNPE_ROOT/lib/hexagon-v75/unsigned/libSnpeHtpV75Skel.so ./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/.
cp $SNPE_ROOT/lib/hexagon-v79/unsigned/libSnpeHtpV79Skel.so ./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/.


# Validate if all libs are copied
[ -f "./SentimentAnalysis/app/src/main/cmakeLibs/arm64-v8a/libSNPE.so" ] && echo "True" || echo "Lib not found, please copy manually"
[ -f "./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/libSnpeHtpPrepare.so" ] && echo "True" || echo "Lib not found, please copy manually"
[ -f "./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/libSnpeHtpV69Skel.so" ] && echo "True" || echo "Lib not found, please copy manually"
[ -f "./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/libSnpeHtpV69Stub.so" ] && echo "True" || echo "Lib not found, please copy manually"
[ -f "./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/libSnpeHtpV73Skel.so" ] && echo "True" || echo "Lib not found, please copy manually"
[ -f "./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/libSnpeHtpV73Stub.so" ] && echo "True" || echo "Lib not found, please copy manually"
[ -f "./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/libSnpeHtpV75Skel.so" ] && echo "True" || echo "Lib not found, please copy manually"
[ -f "./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/libSnpeHtpV75Stub.so" ] && echo "True" || echo "Lib not found, please copy manually"
[ -f "./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/libSnpeHtpV79Skel.so" ] && echo "True" || echo "Lib not found, please copy manually"
[ -f "./SentimentAnalysis/app/src/main/jniLibs/arm64-v8a/libSnpeHtpV79Stub.so" ] && echo "True" || echo "Lib not found, please copy manually"

