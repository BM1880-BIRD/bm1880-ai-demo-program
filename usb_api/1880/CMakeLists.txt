cmake_minimum_required(VERSION 2.8)
project(bm1880_usb_test)
add_executable(bm1880_usb_test bm1880_usb_test.c bm1880_usb.c)

include(CMakeForceCompiler)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR arm)

# specify the cross compiler
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(CROSS_COMPILE "aarch64-linux-gnu-")

#set(OURSYSROOT $ENV{SDKTARGETSYSROOT})
#set(CMAKE_C_FLAGS "-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=softfp -mmusl --sysroot=${OURSYSROOT} ${CMAKE_C_FLAGS}")
#set(CMAKE_CXX_FLAGS "-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=softfp -mmusl --sysroot=${OURSYSROOT} ${CMAKE_CXX_FLAGS}")

# where is the target environment
set(CMAKE_FIND_ROOT_PATH $ENV{SDKTARGETSYSROOT})

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# set current model
set(RTCFALCON_MODEL ARGUS)
