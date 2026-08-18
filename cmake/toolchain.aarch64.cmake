# Cross build toolchain for 64-bit ARM targets.
#
#   export DX_SYSROOT=/path/to/target/sysroot   # holds DXRT + OpenCV for the target
#   ./build.sh --arch aarch64 --dxrt-dir ${DX_SYSROOT}/usr
#
# A Yocto build does not use this file: the cmake bbclass generates its own
# toolchain from the recipe sysroot.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

if(DEFINED ENV{DX_SYSROOT})
    set(CMAKE_SYSROOT $ENV{DX_SYSROOT})
    set(CMAKE_FIND_ROOT_PATH $ENV{DX_SYSROOT})
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BEFORE)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
