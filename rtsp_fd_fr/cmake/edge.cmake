if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(${PLATFORM} STREQUAL "cmodel")
        set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/platforms/linux/gnu.toolchain.cmake)
    elseif(${PLATFORM} STREQUAL "soc_bm1880_asic")
        set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/platforms/linux/aarch64-gnu.toolchain.cmake)
    else()
        message(fatal "platform is unsupported")
    endif()
endif()

include(${CMAKE_SOURCE_DIR}/cmake/library/${PLATFORM}.cmake)

set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
if(DEFINED CUSTOM_PREBUILT_DIR)
    set(GTEST_LIBRARIES "${CUSTOM_PREBUILT_DIR}/lib/libgtest.a")
else()
    set(GTEST_LIBRARIES "${PREBUILT_DIR}/${PLATFORM}/lib/libgtest.a")
endif()

set(PLATFORM_INCLUDE_DIRS
    ${PREBUILT_INCLUDE_DIR}/
    ${PREBUILT_INCLUDE_DIR}/bm_networks
    ${PREBUILT_INCLUDE_DIR}/bm_networks/utils
    ${PREBUILT_INCLUDE_DIR}/gflags
    ${PREBUILT_INCLUDE_DIR}/glog
    ${PREBUILT_INCLUDE_DIR}/google
    ${PREBUILT_INCLUDE_DIR}/opencv2
    ${PREBUILT_INCLUDE_DIR}/openpose
)

set (PLATFORM_LDFLAGS
    -L${PREBUILT_LIBRARY_DIR}
    ${ESSENTIAL_LDFLAGS}
    ${FFMPEG_LDFLAGS}
    ${NETWORK_LDFLAGS}
    ${DECODE_LDFLAGS}
    ${OPENCV_LDFLAGS}
)

set(MODULES "face_server;pipeline_preset;inference;zktcam;fdfr;utils;usb_io;io;opencv_app;pose;${PLATFORM_SPECIFIC_MODULES}")
