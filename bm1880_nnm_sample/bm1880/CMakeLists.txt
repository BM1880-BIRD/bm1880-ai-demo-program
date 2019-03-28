cmake_minimum_required(VERSION 3.2.2)
project(ttyservice)

cmake_minimum_required(VERSION 3.0)
find_package(PkgConfig REQUIRED)

set(ARCH "aarch64-linux-gnu-")

set (TOOLCHAIN "" CACHE STRING "Enter gcc toolchain path")
set_property(
    CACHE TOOLCHAIN
    PROPERTY STRINGS
    "Option A" "Option B" "Option C"
)
#set (TOOLCHAIN_PATH /gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin)
#set (BMIVA_PATH /bmiva)

set (CMAKE_CXX_COMPILER ${TOOLCHAIN_PATH}/${ARCH}g++)
set (CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/${ARCH}gcc)
set (BMENGINE_HEADERS ${BMIVA_PATH}/include)
set (BMIVA_FACE_HEADERS ${BMIVA_PATH}/include)
set (BMIVA_UTILS_HEADERS ${BMIVA_PATH}/include)
set (BMIVA_INCLUDE_PATH ${BMIVA_PATH}/include)

set (BMIVA_LD_FLAGS -L${BMIVA_PATH}/lib -lbmiva_engine -lbmiva_face -lbmiva_utils -lbmkernel -lbmruntime -lbmutils -lglog -lbmodel -lprotobuf -ldl -lbmjpuapi -lbmjpulite)
set (OPENCV_INCLUDE_DIRS ${BMIVA_PATH}/include)
set (OPENCV_LDFLAGS -L${BMIVA_PATH}/lib -lopencv_imgcodecs;-lopencv_imgproc;-lopencv_core)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link=${BMIVA_PATH}/lib/")

add_executable(ipc_face_detector ipc_face_detector.cpp bm_usbtty.cpp bm_storage.cpp bm_link.c bm_cquene.cpp sqlite3.c)
target_include_directories(ipc_face_detector PRIVATE ${OPENCV_INCLUDE_DIRS})
target_include_directories(ipc_face_detector PRIVATE ${BMIVA_CONFIG_PATH})
target_include_directories(ipc_face_detector PRIVATE ${BMIVA_INCLUDE_PATH})
target_include_directories(ipc_face_detector PRIVATE ${APP_HEADERS})
target_include_directories(ipc_face_detector PRIVATE ${BMENGINE_HEADERS})
target_include_directories(ipc_face_detector PRIVATE ${BMIVA_FACE_HEADERS})
target_include_directories(ipc_face_detector PRIVATE ${BMIVA_UTILS_HEADERS})
target_include_directories(ipc_face_detector PRIVATE ${BMIVA_GLOG_HEADERS})
target_include_directories(ipc_face_detector PRIVATE ${PRE_BUILT_HEADERS})
target_link_libraries (ipc_face_detector ${OPENCV_LDFLAGS})
target_link_libraries (ipc_face_detector ${BMIVA_LD_FLAGS})
target_link_libraries (ipc_face_detector -lpthread)
