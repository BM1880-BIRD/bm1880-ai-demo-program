cmake_minimum_required(VERSION 3.0)

add_library(3rd INTERFACE)
add_library(3rd_json INTERFACE)
target_include_directories(3rd_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(3rd_annoy INTERFACE)
target_include_directories(3rd_annoy INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/annoy/include)
target_link_libraries(3rd INTERFACE 3rd_json)
target_link_libraries(3rd INTERFACE 3rd_annoy)