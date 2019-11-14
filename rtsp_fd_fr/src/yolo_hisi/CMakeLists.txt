cmake_minimum_required(VERSION 3.0)

set(MODULE_LIBRARIES "io_usb;pipeline_extension;images_core")

set(LIB_CPPS "yolo.cpp")
set(LIBNAME "yolo")
add_library(yolo SHARED ${LIB_CPPS})
target_compile_options(${LIBNAME} PRIVATE -Werror)
target_include_directories(${LIBNAME} PRIVATE ${CMAKE_SOURCE_DIR}/modules/inference/object_detection/include)
target_link_libraries (${LIBNAME} ${MODULE_LIBRARIES})

if (OPENCV_FOUND STREQUAL "1")
    set(PROG_CPPS "main.cpp")
    set(PROGNAME "yolo_hisi")
    add_executable(${PROGNAME} ${LIB_CPPS} ${PROG_CPPS})
    target_compile_options(${PROGNAME} PRIVATE -Werror)
    target_include_directories(${PROGNAME} PRIVATE ${CMAKE_SOURCE_DIR}/modules/inference/object_detection/include)
    target_link_libraries (${PROGNAME} ${MODULE_LIBRARIES})
endif()