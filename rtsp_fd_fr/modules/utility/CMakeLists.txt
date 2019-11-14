cmake_minimum_required(VERSION 3.0)

init_module("utility" INTERFACE)
add_submodule("utility" "utils")
add_submodule("utility" "database")
add_submodule("utility" "logging")

target_link_libraries("utility_utils" PUBLIC 3rd_json)
target_link_libraries("utility_database" PUBLIC 3rd_json)