cmake_minimum_required(VERSION 3.0)
find_package(PkgConfig REQUIRED)

include(${CMAKE_SOURCE_DIR}/cmake/library/shared.cmake)
if (NOT DEFINED ARCH)
    set(ARCH x86_64)
endif()

set(ENV{PKG_CONFIG_PATH} "")

pkg_check_modules(LIBUSB libusb-1.0)
add_library(libusb INTERFACE)
target_include_directories(libusb INTERFACE ${LIBUSB_INCLUDE_DIRS})
target_link_libraries(libusb INTERFACE ${LIBUSB_LDFLAGS})

add_library(glog INTERFACE)
target_link_libraries(glog INTERFACE -lglog)
target_include_directories(glog INTERFACE ${PREBUILT_INCLUDE_DIR}/bm_networks/utils)
add_dependencies(glog prebuilt)
set(GLOG_FOUND "1")

target_link_libraries(prebuilt_network INTERFACE -L${PREBUILT_LIBRARY_DIR} -lopenblas)

pkg_check_modules(OPENCV opencv)
add_library(opencv INTERFACE)
target_include_directories(opencv INTERFACE ${OPENCV_INCLUDE_DIRS})
target_link_libraries(opencv INTERFACE ${OPENCV_LDFLAGS})
