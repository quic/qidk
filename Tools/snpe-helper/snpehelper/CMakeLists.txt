#==============================================================================
#
#  Copyright (c) 2020-2023 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#
#==============================================================================

cmake_minimum_required (VERSION 3.14)
project (snpehelper)
set (APP "snpehelper")
 option(BUILD_WITH_VCRUNTIME "Build the snpe-sample with static vcruntime libraries." ON)
 message("Build snpe-sample with vcruntime: ${BUILD_WITH_VCRUNTIME}")

set( APP_SOURCES
    "dllmain.cpp"
    "snpehelper.cpp"
    "SNPERuntime.cpp"
)
set (SNPE_ROOT "C:/Qualcomm/AIStack/SNPE/2.15.1.230926")
set (SNPE_INCLUDE_DIR "${SNPE_ROOT}/include/SNPE")
set (SNPE_LIB_PREFIX "${SNPE_ROOT}/lib")
set (_dtuple_POSTFIX "windows-msvc")
set (PYTHON_INCLUDE_DIR "C:/Users/HCKTest/AppData/Local/Programs/Python/Python38/include")
set (PYTHON_LIB_DIR "C:/Users/HCKTest/AppData/Local/Programs/Python/Python38/libs")
set (PYBIND11_INCLUDE_DIR "C:/Users/HCKTest/AppData/Local/Programs/Python/Python38/lib/site-packages/pybind11/include")

set(RELEASE "Release/")
set(CMAKE_CXX_STANDARD 17)

message("Linking with ARM64 SNPE")
get_filename_component(SNPE_IMPLIB_PATH "${SNPE_LIB_PREFIX}/arm64x-${_dtuple_POSTFIX}/SNPE.lib" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(SNPE_DLL_PATH "${SNPE_LIB_PREFIX}/arm64x-${_dtuple_POSTFIX}/SNPE.dll" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(SNPE_HTP_PATH "${SNPE_LIB_PREFIX}/arm64x-${_dtuple_POSTFIX}/SnpeHtpPrepare.dll" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
if(CHIPSET STREQUAL "SC8380")
	message("SC8380 is selected")
	get_filename_component(SNPE_STUB_PATH "${SNPE_LIB_PREFIX}/arm64x-${_dtuple_POSTFIX}/SnpeHtpV73Stub.dll" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
	get_filename_component(SNPE_SKEL_PATH "${SNPE_LIB_PREFIX}/hexagon-v73/unsigned/libSnpeHtpV73Skel.so" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
else()
	message("Default is selected")
	get_filename_component(SNPE_STUB_PATH "${SNPE_LIB_PREFIX}/arm64x-${_dtuple_POSTFIX}/SnpeHtpV68Stub.dll" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
	get_filename_component(SNPE_SKEL_PATH "${SNPE_LIB_PREFIX}/hexagon-v68/unsigned/libSnpeHtpV68Skel.so" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

add_library(SNPE SHARED IMPORTED)
set_target_properties(SNPE PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES ${SNPE_INCLUDE_DIR}  
	IMPORTED_IMPLIB ${SNPE_IMPLIB_PATH}
	IMPORTED_LOCATION ${SNPE_DLL_PATH}
)
include_directories(${PYTHON_INCLUDE_DIR})
include_directories(${PYBIND11_INCLUDE_DIR})
link_directories(${PYTHON_LIB_DIR})

add_library(${APP} MODULE ${APP_SOURCES})

target_compile_definitions(${APP} PUBLIC -D_CRT_SECURE_NO_WARNINGS)
 if(${BUILD_WITH_VCRUNTIME})
     target_compile_options(${APP} PUBLIC /MT)
 endif()

target_link_libraries (${APP} PUBLIC SNPE)

add_custom_command(TARGET ${APP} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	${CMAKE_CURRENT_SOURCE_DIR}/build/Release/${APP}.dll 
	${CMAKE_CURRENT_SOURCE_DIR}/build/Release/${APP}.pyd)
