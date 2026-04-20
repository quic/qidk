# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.18.1)

# Add headers
#set(CMAKE_CXX_STANDARD 20)  # Set the C++20 standard
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/genie/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# Declares and names the project.
project("aiassistant")


add_library( # Sets the name of the library.
        aiassistant
        # Sets the library as a shared library.
        SHARED
        native-lib.cpp)
#		genie_inference.cpp)



find_library( # Sets the name of the path variable.
        log-lib
        android-lib
        android
        log)


target_link_libraries(
        aiassistant
        ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/arm64-v8a/libGenie.so
        ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/arm64-v8a/libQnnHtp.so
#        ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/arm64-v8a/libQnnSystem.so
#        ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/arm64-v8a/libQnnHtpPrepare.so
#        ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/arm64-v8a/libQnnHtpNetRunExtensions.so
#        ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/arm64-v8a/libQnnHtpV79Stub.so
#        ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/arm64-v8a/
        android
        log)