cmake_minimum_required(VERSION 3.5)
project(mpp_app)

# set install path 
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/install)
set(PREBUILT_DIR ${PROJECT_SOURCE_DIR}/prebuilt)

# set the C++ standard to C++ 11
set(CMAKE_CXX_STANDARD 11)


# set compiler tool
#set(HISIARM_TOOLCHAIN_BIN /opt/hisi-linux/x86-arm/arm-hisiv300-linux/bin/)
if (NOT DEFINED HISIARM_TOOLCHAIN_BIN)
    set (HISIARM_TOOLCHAIN_BIN "")
endif()
set(ARM arm-hisiv300-linux-uclibcgnueabi-)
set(CMAKE_C_COMPILER ${HISIARM_TOOLCHAIN_BIN}${ARM}gcc)
set(CMAKE_CXX_COMPILER ${HISIARM_TOOLCHAIN_BIN}${ARM}g++)
set(CMAKE_CXX_FLAGS "-mcpu=cortex-a7")


# add sensor type
add_definitions(-DSENSOR_TYPE=SONY_IMX290_MIPI_1080P_30FPS)
set(pre_lib libVoiceEngine libupvqe 
    libdnvqe libdnvqe libmpi libisp 
    libsns_imx290 lib_cmoscfg lib_iniparser 
    lib_hiae lib_hiawb lib_hiaf lib_hidefog )

# add prebuilt library

add_library(libVoiceEngine SHARED IMPORTED)
set_target_properties(libVoiceEngine PROPERTIES
  IMPORTED_LOCATION "${PREBUILT_DIR}/libVoiceEngine.so"
)
add_library(libupvqe SHARED IMPORTED)
set_target_properties(libupvqe PROPERTIES
  IMPORTED_LOCATION "${PREBUILT_DIR}/libupvqe.so"
)
add_library(libdnvqe SHARED IMPORTED)
set_target_properties(libdnvqe PROPERTIES
  IMPORTED_LOCATION "${PREBUILT_DIR}/libdnvqe.so"
)
add_library(libmpi SHARED IMPORTED)
set_target_properties(libmpi PROPERTIES
  IMPORTED_LOCATION "${PREBUILT_DIR}/libmpi.so"
)
add_library(libisp SHARED IMPORTED)
set_target_properties(libisp PROPERTIES
  IMPORTED_LOCATION "${PREBUILT_DIR}/libisp.so"
)
add_library(libsns_imx290 SHARED IMPORTED)
set_target_properties(libsns_imx290 PROPERTIES
  IMPORTED_LOCATION "${PREBUILT_DIR}/libsns_imx290.so"
)
add_library(lib_cmoscfg SHARED IMPORTED)
set_target_properties(lib_cmoscfg PROPERTIES
  IMPORTED_LOCATION "${PREBUILT_DIR}/lib_cmoscfg.so"
)
add_library(lib_iniparser SHARED IMPORTED)
set_target_properties(lib_iniparser PROPERTIES
  IMPORTED_LOCATION "${PREBUILT_DIR}/lib_iniparser.so"
)
add_library(lib_hiae SHARED IMPORTED)
set_target_properties(lib_hiae PROPERTIES
  IMPORTED_LOCATION "${PREBUILT_DIR}/lib_hiae.so"
)
add_library(lib_hiawb SHARED IMPORTED)
set_target_properties(lib_hiawb PROPERTIES
  IMPORTED_LOCATION "${PREBUILT_DIR}/lib_hiawb.so"
)
add_library(lib_hiaf SHARED IMPORTED)
set_target_properties(lib_hiaf PROPERTIES
  IMPORTED_LOCATION "${PREBUILT_DIR}/lib_hiaf.so"
)
add_library(lib_hidefog SHARED IMPORTED)
set_target_properties(lib_hidefog PROPERTIES
  IMPORTED_LOCATION "${PREBUILT_DIR}/lib_hidefog.so"
)

add_library(libyolo SHARED IMPORTED)
set_target_properties(libyolo PROPERTIES
  IMPORTED_LOCATION "${PREBUILT_DIR}/libyolo.so"
)

add_library(libusb-1.0 SHARED IMPORTED)
set_target_properties(libusb-1.0 PROPERTIES
  IMPORTED_LOCATION "${PREBUILT_DIR}/libusb-1.0.so"
  INTERFACE_INCLUDE_DIRECTORIES "${PREBUILT_DIR}/include"
)

# add pthread lib
find_package(Threads)

# common source 
set(COMM_SRCS 
      common/sample_comm_audio.c
      common/sample_comm_isp.c
      common/sample_comm_ive.c
      common/sample_comm_sys.c
      common/sample_comm_vda.c
      common/sample_comm_venc.c
      common/sample_comm_vi.c
      common/sample_comm_vo.c
      common/sample_comm_vpss.c
)

add_executable(bm_mpp_system src/demo.cpp src/bm_mpp_system.cpp ${COMM_SRCS} ${PREBUILT_LIB})
target_link_libraries(bm_mpp_system  ${PREBUILT_DIR}/libcurl/lib/libcurl.a ${pre_lib} ${CMAKE_THREAD_LIBS_INIT} libyolo libusb-1.0)

target_include_directories(bm_mpp_system
    PRIVATE
        ${PROJECT_SOURCE_DIR}/common/
        ${PROJECT_SOURCE_DIR}/include
        ${PREBUILT_DIR}/include
        ${PREBUILT_DIR}/libcurl/include
        )
