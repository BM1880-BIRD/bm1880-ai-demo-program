cmake_minimum_required(VERSION 2.8)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR arm)

# specify the cross compiler
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(CROSS_COMPILE "aarch64-linux-gnu-")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-result -Wno-unknown-pragmas -Wfatal-errors -fPIC")

set(USB_API_DIR ../../usb_api)
set(DARKNET_DIR ../../darknet-yolov2-object-classification-v2/darknet)
set(DARKNET_SRC_DIR ${DARKNET_DIR}/src)
set(DARKNET_INC_DIR ${DARKNET_DIR}/include)
set(BM_INC_DIR ../../../bm1880-system-sdk/middleware/include)
set(BM_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../bm1880-system-sdk/middleware/lib)
project(darknet_server)
file(GLOB BM1880USB_SRC ${USB_API_DIR}/1880/bm1880_usb.c)
file(GLOB DARKNET_SRC ${DARKNET_SRC_DIR}/*.c)
list(FILTER DARKNET_SRC EXCLUDE REGEX ".*._local_layer.c")
list(FILTER DARKNET_SRC EXCLUDE REGEX ".*compare.c")
link_directories(${BM_LIB_DIR})
add_library(libbm1880_usb OBJECT ${BM1880USB_SRC})
add_library(libdarknet OBJECT ${DARKNET_SRC})
add_executable(darknet_server darknet_server.c $<TARGET_OBJECTS:libbm1880_usb> $<TARGET_OBJECTS:libdarknet>)
target_include_directories(libbm1880_usb PRIVATE ${USB_API_DIR}/1880)
target_include_directories(libdarknet PRIVATE ${DARKNET_SRC_DIR} ${DARKNET_INC_DIR} ${BM_INC_DIR})
target_include_directories(darknet_server PRIVATE ${USB_API_DIR}/1880 ${DARKNET_SRC_DIR} ${DARKNET_INC_DIR} ${BM_INC_DIR})
target_link_libraries(darknet_server m pthread bmutils bmruntime bmodel bmkernel glog stdc++ protobuf cblas_LINUX)



