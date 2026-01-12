cmake_minimum_required(VERSION 3.10)

# Set the project name
project(prepare-native)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCE_FILES
        "./*.cpp"
)


# Create the shared library
add_library(prepare-native SHARED ${SOURCE_FILES})

target_link_libraries(prepare-native
        android
        log
        GLESv2
        EGL
        c
        m
        dl
)