cmake_minimum_required(VERSION 3.6)

include("../config.cmake")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LRS_ANDROID_V4L2_ROOT_DIR}/libs/${ANDROID_ABI})
add_subdirectory(${LRS_ROOT_DIR} ${LRS_ANDROID_V4L2_ROOT_DIR}/build-${ANDROID_ABI})
