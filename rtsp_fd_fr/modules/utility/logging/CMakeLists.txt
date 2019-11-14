cmake_minimum_required(VERSION 3.0)

include(${CMAKE_SOURCE_DIR}/modules/3rd/tracer/tracer.cmake)

set (MODULE utility_logging)

add_library(${MODULE} INTERFACE)
target_include_directories(${MODULE} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if (GLOG_FOUND STREQUAL "1")
    target_link_libraries(${MODULE} INTERFACE glog)
else()
    target_compile_definitions(${MODULE} INTERFACE GLOG_DISABLED)
endif()
