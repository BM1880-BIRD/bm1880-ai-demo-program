cmake_minimum_required(VERSION 3.0)

set(MODULE_LIBRARIES "images;inference;io;pipeline;utility")

file(GLOB PROG_CPPS *.cpp)
foreach (PROG_CPP ${PROG_CPPS})
    get_filename_component(PROG_NAME ${PROG_CPP} NAME_WE ${Tracer_SRC})
    add_executable(${PROG_NAME} ${PROG_NAME}.cpp)

    target_compile_options(${PROG_NAME} PRIVATE -Werror)

    target_include_directories(${PROG_NAME} PRIVATE ${PREBUILT_INCLUDE_DIRS})
    target_include_directories(${PROG_NAME} PRIVATE ${Tracer_INCLUDE_DIRS})

    target_link_libraries (${PROG_NAME} ${MODULE_LIBRARIES})
endforeach()
