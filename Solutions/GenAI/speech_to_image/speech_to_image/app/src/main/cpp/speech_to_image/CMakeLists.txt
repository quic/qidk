cmake_minimum_required(VERSION 3.10)

# Set the project name
project(speech_to_image-native)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the source files
# Using more specific patterns to include both .cpp and .hpp files
file(GLOB_RECURSE SOURCE_FILES
        "./*.cpp"
        "./src/*.cpp"
        "./src/Log/*.cpp"
        "./src/Utils/*.cpp"
        "./src/WrapperUtils/*.cpp"
        "./src/PAL/src/linux/*.cpp"
        "./src/PAL/src/commons/*.cpp"
        "./src/LogMel/src/*.cpp"
)

# Create the shared library
add_library(speech_to_image-native SHARED ${SOURCE_FILES})


set(QNN_SDK_ROOT $ENV{QNN_SDK_ROOT})

# Include directories for header files
include_directories(
        ./
        ./src
        ./src/Log
        ./src/PAL/include
        ./src/PAL/include/PAL
        ./src/Utils
        ./src/WrapperUtils
        ./src/LogMel/include
        ./src//QNN
)

target_link_libraries(speech_to_image-native
        android
        log
        GLESv2
        EGL
        c
        m
        dl
        ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/libQnnHtp.so
        ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/libQnnHtpPrepare.so
        ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/libQnnSystem.so
        ${CMAKE_SOURCE_DIR}/../jniLibs/arm64-v8a/libc++_shared.so
  )


