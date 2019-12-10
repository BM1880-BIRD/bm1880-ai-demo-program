cmake_minimum_required(VERSION 3.0)
find_package(PkgConfig REQUIRED)

set (MODULE_LIBRARIES "images;io;utility;service_face_client")

file(GLOB PROG_CPPS *.cpp)
foreach (PROG_CPP ${PROG_CPPS})
    get_filename_component(PROG_NAME ${PROG_CPP} NAME_WE)
    add_executable(${PROG_NAME} ${PROG_NAME}.cpp)

    target_compile_options(${PROG_NAME} PRIVATE -Werror)

    target_include_directories(${PROG_NAME} PRIVATE ${PREBUILT_INCLUDE_DIRS})
    target_include_directories(${PROG_NAME} PRIVATE ${MODULE_INCLUDE_DIRS})

    target_link_libraries (${PROG_NAME} ${MODULE_LIBRARIES})
endforeach()

