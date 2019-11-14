cmake_minimum_required(VERSION 3.0)
find_package(PkgConfig REQUIRED)

include (${CMAKE_SOURCE_DIR}/cmake/library/shared.cmake)

if (NOT DEFINED ARCH)
    set(ARCH hisiv300)
endif()

set(ENV{PKG_CONFIG_PATH} ${PREBUILT_LIBRARY_DIR}/pkgconfig)

pkg_check_modules(LIBUSB libusb-1.0)
add_library(libusb INTERFACE)
target_include_directories(libusb INTERFACE ${LIBUSB_INCLUDE_DIRS})
target_link_libraries(libusb INTERFACE ${LIBUSB_LDFLAGS})

add_library(glog INTERFACE)
target_link_libraries(glog INTERFACE -L${PREBUILT_LIBRARY_DIR} -lglog)
target_include_directories(glog INTERFACE ${PREBUILT_INCLUDE_DIR}/bm_networks/utils)
target_include_directories(glog INTERFACE ${PREBUILT_INCLUDE_DIR}/glog)
target_include_directories(glog INTERFACE ${PREBUILT_INCLUDE_DIR}/gflags)
add_dependencies(glog prebuilt)
set(GLOG_FOUND "1")

add_compile_options (-DOPENCV_DISABLED)
