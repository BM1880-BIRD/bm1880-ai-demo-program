cmake_minimum_required(VERSION 3.0)
find_package(PkgConfig REQUIRED)

include(${CMAKE_SOURCE_DIR}/cmake/library/shared.cmake)
if (NOT DEFINED ARCH)
    set(ARCH aarch64)
endif()

add_library(glog INTERFACE)
target_link_libraries(glog INTERFACE -L${PREBUILT_LIBRARY_DIR} -lglog)
target_include_directories(glog INTERFACE ${PREBUILT_INCLUDE_DIR}/bm_networks/utils)
target_include_directories(glog INTERFACE ${PREBUILT_INCLUDE_DIR}/glog)
target_include_directories(glog INTERFACE ${PREBUILT_INCLUDE_DIR}/gflags)
add_dependencies(glog prebuilt)
set(GLOG_FOUND "1")

add_library(libusb INTERFACE)

set(FFMPEG_LDFLAGS
    -L${PREBUILT_LIBRARY_DIR}
    -lpostproc
    -lavcodec
    -lavformat
    -lavdevice
    -lavfilter
    -lavutil
    -lswscale
    -lswresample
    -lbmvideo
    -lbmjpulite
    -lbmjpuapi
)

set(OPENCV_LDFLAGS
    -L${PREBUILT_LIBRARY_DIR}
    ${FFMPEG_LDFLAGS}
    -lopencv_videoio
    -lopencv_imgcodecs
    -lopencv_video
    -lopencv_photo
    -lopencv_imgproc
    -lopencv_core
    -lopencv_highgui
    -lvideo_bm
    -lopencv_ml
)

target_link_libraries(prebuilt_network INTERFACE
    -L${PREBUILT_LIBRARY_DIR}
    -lcblas_LINUX
)

add_library(opencv INTERFACE)
target_include_directories(opencv INTERFACE ${PREBUILT_INCLUDE_DIR}/opencv2)
target_link_libraries(opencv INTERFACE ${OPENCV_LDFLAGS})
add_dependencies(opencv prebuilt)
