cmake_minimum_required(VERSION 3.10)
project(demo)

set(CMAKE_SYSTEM_NAME QNX)
set(QNX_HOST "$ENV{QNX_HOST}")
if(NOT QNX_HOST)
    message(FATAL_ERROR "Environment variable QNX_HOST not set!")
endif()
set(QNX_TARGET "$ENV{QNX_TARGET}")
if(NOT QNX_TARGET)
    message(FATAL_ERROR "Environment variable QNX_TARGET not set!")
endif()



set(QNX_COMPILER_TARGET "")
set(QNX_PROCESSOR aarch64)
set(CMAKE_C_COMPILER ${QNX_HOST}/usr/bin/qcc)
set(CMAKE_C_COMPILER_TARGET ${QNX_COMPILER_TARGET})
set(CMAKE_CXX_COMPILER ${QNX_HOST}/usr/bin/q++)
set(CMAKE_CXX_COMPILER_TARGET ${QNX_COMPILER_TARGET})

include_directories(third_part/rpcmem/)

set(LINK_DIR third_part/libs)
link_directories(${LINK_DIR})

add_executable(demo main.c)

target_link_libraries(demo stdc++ qhci fastrpc_pmem libstd smmu_client cdsprpc)

