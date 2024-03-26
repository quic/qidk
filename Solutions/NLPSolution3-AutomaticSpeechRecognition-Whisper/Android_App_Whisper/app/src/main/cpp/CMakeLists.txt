# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

project("ASR")


# Declares and names the project.

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc/zdl)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc/hpp)

# Build the main target `native-lib`
add_library(ASR SHARED native-lib_pre_post.cpp inference.cpp inference_helper.cpp)

find_library( log-lib log ) # Library required by NDK.
find_library(android-lib android) # for AssetManager functionality

target_link_libraries(ASR ${log-lib} ${android-lib} ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/arm64-v8a/libSNPE.so
        android
        log)