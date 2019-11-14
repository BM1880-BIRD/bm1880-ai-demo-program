cmake_minimum_required(VERSION 3.0)

include(${CMAKE_SOURCE_DIR}/modules/3rd/tracer/tracer.cmake)

set (MODULE io_usb)

if (${PLATFORM} STREQUAL "soc_bm1880_asic")
    file(GLOB SOURCES src/*.cpp src/edge/*.cpp)
else ()
    file(GLOB SOURCES src/*.cpp src/host/*.cpp)
endif ()

add_library(${MODULE} STATIC EXCLUDE_FROM_ALL ${SOURCES} ${Tracer_SRC})
target_include_directories(${MODULE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${MODULE} PUBLIC libusb)
target_link_libraries(${MODULE} PUBLIC ${STANDARD_LDFLAGS})
target_link_libraries(io_usb PUBLIC io_core)
target_link_libraries(io_usb INTERFACE utility_logging)
