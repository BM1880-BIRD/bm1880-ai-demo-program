cmake_minimum_required(VERSION 3.0)

set (CMAKE_FIND_LIBRARY_PREFIXES "")
set (CMAKE_FIND_LIBRARY_SUFFIXES "")
set (PREBUILT_DIR ${CMAKE_SOURCE_DIR}/prebuilt)
include (${CMAKE_SOURCE_DIR}/cmake/configure.cmake)
include (${CMAKE_SOURCE_DIR}/modules/3rd/tracer/tracer.cmake)
include (${CMAKE_SOURCE_DIR}/modules/3rd/LibLinear/liblinear.cmake)

if (NOT DEFINED PROGRAMS)
    set (PROGRAMS "")
    list(APPEND PROGRAMS ${PROGRAM})
endif()

include(${CMAKE_SOURCE_DIR}/cmake/plc.cmake)
foreach(PROGRAM ${PROGRAMS})
    if (EXISTS ${CMAKE_SOURCE_DIR}/pipelines/${PROGRAM}.pl)
        if ((NOT EXISTS ${CMAKE_SOURCE_DIR}/src/${PROGRAM}) OR (DEFINED REGENERATE))
            message(STATUS "Generate pipeline program ${PROGRAM} with plc")
            plc_generate(${PROGRAM})
        endif()
    endif()
endforeach()

project (bm1880_fdfr)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_FLAGS_RELEASE "-O2")

# Set CMake options
include(${CMAKE_SOURCE_DIR}/cmake/options.cmake)
configure_file(${CMAKE_SOURCE_DIR}/config.h.in config.h)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include (VERSION.cmake)
configure_file(${CMAKE_SOURCE_DIR}/cmake/fdfr_version.h.in ${CMAKE_SOURCE_DIR}/include/fdfr_version.h @ONLY)

add_subdirectory(modules)

foreach(PROGRAM ${PROGRAMS})
    add_subdirectory(src/${PROGRAM})
endforeach()
