# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.18.1)

# Add headers
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc/zdl)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc/hpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# Declares and names the project.
project("ASR")


add_library( # Sets the name of the library.
        ASR
        # Sets the library as a shared library.
        SHARED
        native-lib.cpp inference.cpp inference_helper.cpp)



find_library( # Sets the name of the path variable.
        log-lib
        android-lib
        android
        log)


target_link_libraries(
        ASR
        ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/arm64-v8a/libSNPE.so
        android
        log)