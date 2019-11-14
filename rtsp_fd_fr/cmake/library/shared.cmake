cmake_minimum_required(VERSION 3.0)
find_package(PkgConfig REQUIRED)

if(DEFINED CUSTOM_PREBUILT_DIR)
    set(PREBUILT_INCLUDE_DIR ${CUSTOM_PREBUILT_DIR}/include)
    set(PREBUILT_LIBRARY_DIR ${CUSTOM_PREBUILT_DIR}/lib)
else()
    set(PREBUILT_INCLUDE_DIR ${PREBUILT_DIR}/${PLATFORM}/include)
    set(PREBUILT_LIBRARY_DIR ${PREBUILT_DIR}/${PLATFORM}/lib)
endif()

set(CMAKE_PREFIX_PATH ${PREBUILT_LIBRARY_DIR}/cmake/qnn)
include(${CMAKE_SOURCE_DIR}/cmake/dependence.cmake)
find_package(qnn ${MIN_QNN_VERSION} QUIET)

set(STANDARD_LDFLAGS -lpthread -lm)

add_library(prebuilt_network INTERFACE)
add_dependencies(prebuilt_network prebuilt)
add_dependencies(prebuilt_network qnn)

target_include_directories(prebuilt_network INTERFACE
    ${PREBUILT_INCLUDE_DIR}/
    ${PREBUILT_INCLUDE_DIR}/bm_networks
    ${PREBUILT_INCLUDE_DIR}/bm_networks/utils
)
target_link_libraries(prebuilt_network INTERFACE
    -L${PREBUILT_LIBRARY_DIR}
    -lbmkernel
    -lbmodel
    -lbmruntime
    -lbmutils
    -lbmblas
    -lqnn_facepose
    -lqnn_mobilenet_ssd
    -lqnn_mtcnn
    -lqnn_networks_core
    -lqnn_reid
    -lqnn_ssh
    -lqnn_face_attribute
    -lqnn_networks_utils
    -lqnn_antifacespoofing
    -lqnn_openpose
    -lopenpose_lite
    -lprotobuf
    -lqnn_yolo
)

add_compile_options(-fPIC)
