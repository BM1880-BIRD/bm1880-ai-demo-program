cmake_minimum_required(VERSION 3.0)

init_module("images" INTERFACE)
add_submodule("images" "core")
add_submodule("images" "processing")

if ("${OPENCV_FOUND}" STREQUAL "1")
    target_link_libraries("images_core" PUBLIC opencv)
endif()

target_link_libraries("images_processing" PUBLIC images_core opencv prebuilt_network io_core utility pipeline_core io_http)