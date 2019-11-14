cmake_minimum_required(VERSION 3.0)

set(MODULE_LIBRARIES "io_usb;pipeline_extension;images_core")

set(LIB_CPPS "libusb_inference_client.cpp")
set(LIBNAME "usb_inference_client")
add_library(${LIBNAME} SHARED ${LIB_CPPS})
target_compile_options(${LIBNAME} PRIVATE -Werror)
target_include_directories(${LIBNAME} PRIVATE ${CMAKE_SOURCE_DIR}/modules/inference/object_detection/include)
target_link_libraries (${LIBNAME} ${MODULE_LIBRARIES})

if (OPENCV_FOUND STREQUAL "1")
    set(PROG_CPPS "libusb_inference_client_opencv.cpp")
    set(PROGNAME "libusb_inference_client_opencv")
    add_executable(${PROGNAME} ${LIB_CPPS} ${PROG_CPPS})
    target_compile_options(${PROGNAME} PRIVATE -Werror)
    target_include_directories(${PROGNAME} PRIVATE ${CMAKE_SOURCE_DIR}/modules/inference/object_detection/include)
    target_link_libraries (${PROGNAME} ${MODULE_LIBRARIES})
endif()