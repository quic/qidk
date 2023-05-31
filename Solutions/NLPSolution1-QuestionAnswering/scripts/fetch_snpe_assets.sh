# Check if SNPE_ROOT is set ?
[ -z "$SNPE_ROOT" ] && echo "SNPE_ROOT not set" && exit -1 || echo "SNPE Root = ${SNPE_ROOT}"

# Add DLC to App asssets dir
cp ./frozen_models/electra_small_squad2_cached.dlc ./QuestionAnswering/bert/src/main/assets/
# Validate if DLC is copied
[ -f "./QuestionAnswering/bert/src/main/assets/electra_small_squad2_cached.dlc" ] && echo "True" || echo "DLC not found in assets dir"

# Add SNPE ARM64 libs to App 'cmakeLibs' and 'jniLibs'
mkdir -p ./QuestionAnswering/bert/src/main/cmakeLibs/arm64-v8a
mkdir -p ./QuestionAnswering/bert/src/main/jniLibs/arm64-v8a
cp $SNPE_ROOT/lib/aarch64-android-clang8.0/libSNPE.so ./QuestionAnswering/bert/src/main/cmakeLibs/arm64-v8a/.
cp $SNPE_ROOT/lib/aarch64-android-clang8.0/libSNPE.so ./QuestionAnswering/bert/src/main/jniLibs/arm64-v8a/.
cp $SNPE_ROOT/lib/aarch64-android-clang8.0/libSnpeHtpPrepare.so ./QuestionAnswering/bert/src/main/jniLibs/arm64-v8a/.
cp $SNPE_ROOT/lib/aarch64-android-clang8.0/libSnpeHtpV69Stub.so ./QuestionAnswering/bert/src/main/jniLibs/arm64-v8a/.
cp $SNPE_ROOT/lib/aarch64-android-clang8.0/libSnpeHtpV73Stub.so ./QuestionAnswering/bert/src/main/jniLibs/arm64-v8a/.
cp $SNPE_ROOT/lib/aarch64-android-clang8.0/libc++_shared.so ./QuestionAnswering/bert/src/main/jniLibs/arm64-v8a/.

# Add SNPE DSP libs to App 'jniLibs'
cp $SNPE_ROOT/lib/dsp/libSnpeHtpV69Skel.so ./QuestionAnswering/bert/src/main/jniLibs/arm64-v8a/.
cp $SNPE_ROOT/lib/dsp/libSnpeHtpV73Skel.so ./QuestionAnswering/bert/src/main/jniLibs/arm64-v8a/.

# Validate if all libs are copied
[ -f "./QuestionAnswering/bert/src/main/cmakeLibs/arm64-v8a/libSNPE.so" ] && echo "True" || echo "Lib not found, please copy manually"
[ -f "./QuestionAnswering/bert/src/main/jniLibs/arm64-v8a/libSnpeHtpPrepare.so" ] && echo "True" || echo "Lib not found, please copy manually"
[ -f "./QuestionAnswering/bert/src/main/jniLibs/arm64-v8a/libSnpeHtpV69Skel.so" ] && echo "True" || echo "Lib not found, please copy manually"
[ -f "./QuestionAnswering/bert/src/main/jniLibs/arm64-v8a/libSnpeHtpV69Stub.so" ] && echo "True" || echo "Lib not found, please copy manually"
[ -f "./QuestionAnswering/bert/src/main/jniLibs/arm64-v8a/libSnpeHtpV73Skel.so" ] && echo "True" || echo "Lib not found, please copy manually"
[ -f "./QuestionAnswering/bert/src/main/jniLibs/arm64-v8a/libSnpeHtpV73Stub.so" ] && echo "True" || echo "Lib not found, please copy manually"
[ -f "./QuestionAnswering/bert/src/main/jniLibs/arm64-v8a/libc++_shared.so" ] && echo "True" || echo "Lib not found, please copy manually"

