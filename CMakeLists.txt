cmake_minimum_required(VERSION 3.14)

project(dx_yolo26
    VERSION 1.0.0
    DESCRIPTION "DEEPX YOLO26 application package"
    LANGUAGES CXX)

get_filename_component(PROJECT_ROOT ${CMAKE_SOURCE_DIR} ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(GNUInstallDirs)

# =============================================================================
# Options
# =============================================================================
# Which runner variant of every application is built. The Yocto recipe builds
# "async" only; a workstation build usually wants both.
set(DXYOLO26_VARIANTS "both" CACHE STRING "Applications to build: both | sync | async")
set_property(CACHE DXYOLO26_VARIANTS PROPERTY STRINGS both sync async)

option(DXYOLO26_WITH_SHAREDLIB "Build the yolov26 postprocess shared library" ON)

# Baked into the applications as PROJECT_ROOT_DIR. It is the directory the
# applications read config/model_registry.json from at run time, so it has to be
# the *installed* data directory for a packaged build (the Yocto recipe passes
# ${datadir}/dx_yolo26). The default keeps an in-tree build runnable as is.
set(DXYOLO26_DATA_DIR "${PROJECT_ROOT}" CACHE PATH
    "Run-time data directory holding config/model_registry.json")

set(DXRT_INSTALLED_DIR "/usr/local" CACHE PATH "DXRT (DeepX Runtime) install prefix")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# Dependencies
# =============================================================================
find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)

# libdxrt is located by path instead of by its CMake package config: that works
# the same way for a native build and for a cross build against a sysroot.
find_library(DXRT_LIBRARY NAMES dxrt REQUIRED
    HINTS ${DXRT_INSTALLED_DIR}/${CMAKE_INSTALL_LIBDIR} ${DXRT_INSTALLED_DIR}/lib)
find_path(DXRT_INCLUDE_DIR NAMES dxrt/dxrt_api.h REQUIRED
    HINTS ${DXRT_INSTALLED_DIR}/include)

# GCC/Clang older than 9 need <experimental/filesystem> to be linked explicitly.
set(DXYOLO26_FS_LIB "")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    set(DXYOLO26_FS_LIB stdc++fs)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    set(DXYOLO26_FS_LIB c++fs)
endif()

add_compile_options(-Wall -Wextra -O3 -fPIC)

add_subdirectory(src/cpp_example)
if(DXYOLO26_WITH_SHAREDLIB)
    add_subdirectory(src/postprocess)
endif()

# =============================================================================
# Run-time data
# =============================================================================
# model_registry.json resolves an application's default .dxnn name when -m is
# omitted; the per-model config.json files carry the postprocess thresholds.
install(FILES config/model_registry.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/config)

file(GLOB_RECURSE _MODEL_CONFIGS RELATIVE ${CMAKE_SOURCE_DIR}/src/cpp_example
     ${CMAKE_SOURCE_DIR}/src/cpp_example/*/*/config.json)
foreach(_CFG ${_MODEL_CONFIGS})
    get_filename_component(_CFG_DIR ${_CFG} DIRECTORY)
    install(FILES src/cpp_example/${_CFG}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/examples/${_CFG_DIR})
endforeach()

# Sample inputs. An application called without -i/-v/-c/-r falls back to
# "sample/img/<task sample>" relative to the working directory, so running from
# the installed data directory works out of the box.
option(DXYOLO26_INSTALL_SAMPLES "Install the sample input images" ON)
if(DXYOLO26_INSTALL_SAMPLES)
    install(DIRECTORY sample
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
endif()

message(STATUS "dx_yolo26 ${PROJECT_VERSION}")
message(STATUS "  build type          : ${CMAKE_BUILD_TYPE}")
message(STATUS "  compiler            : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  system processor    : ${CMAKE_SYSTEM_PROCESSOR} (host ${CMAKE_HOST_SYSTEM_PROCESSOR})")
message(STATUS "  dxrt library        : ${DXRT_LIBRARY}")
message(STATUS "  dxrt headers        : ${DXRT_INCLUDE_DIR}")
message(STATUS "  OpenCV              : ${OpenCV_VERSION}")
message(STATUS "  variants            : ${DXYOLO26_VARIANTS}")
message(STATUS "  postprocess library : ${DXYOLO26_WITH_SHAREDLIB}")
message(STATUS "  run-time data dir   : ${DXYOLO26_DATA_DIR}")
message(STATUS "  install prefix      : ${CMAKE_INSTALL_PREFIX}")
