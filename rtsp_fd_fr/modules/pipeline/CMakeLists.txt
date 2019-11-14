cmake_minimum_required(VERSION 3.0)

init_module("pipeline" INTERFACE)
add_submodule("pipeline" "core")
add_submodule("pipeline" "extension")
add_submodule("pipeline" "dynamic")
target_link_libraries("pipeline_extension" PUBLIC pipeline_core)
target_link_libraries("pipeline_dynamic" PUBLIC pipeline_extension io inference images utility zktcam)