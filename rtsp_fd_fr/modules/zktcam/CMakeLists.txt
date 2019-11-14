cmake_minimum_required(VERSION 3.0)

init_module("zktcam" INTERFACE)
add_submodule("zktcam" "zkt")

file(GLOB_RECURSE DEP_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/zkt/lib/${PLATFORM}/*.so)
target_link_libraries("zktcam_zkt" PUBLIC images ${DEP_LIBRARIES})