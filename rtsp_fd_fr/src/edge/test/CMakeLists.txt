cmake_minimum_required(VERSION 3.0)
find_package(PkgConfig REQUIRED)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
file(GLOB PROG_CPPS *.cpp)
foreach (PROG_CPP ${PROG_CPPS})
    get_filename_component(PROG_NAME ${PROG_CPP} NAME_WE)
    add_executable(${PROG_NAME} ${PROG_NAME}.cpp ${LIBLINEAR_SRC})

    target_compile_options(${PROG_NAME} PRIVATE -Werror)

    target_include_directories(${PROG_NAME} PRIVATE ${PREBUILT_INCLUDE_DIRS})
    target_include_directories(${PROG_NAME} PRIVATE ${MODULE_INCLUDE_DIRS})
    target_include_directories(${PROG_NAME} PRIVATE ${LIBLINEAR_INCLUDE_DIRS})

    target_link_libraries (${PROG_NAME} ${MODULE_LIBRARIES})
    target_link_libraries (${PROG_NAME} ${PLATFORM_LDFLAGS})
endforeach()

target_link_libraries(test_repo ${GTEST_LIBRARIES})
add_test(test_repo ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_repo)

target_link_libraries(test_repo_mutation ${GTEST_LIBRARIES})
add_test(test_repo_mutation ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_repo_mutation)

target_link_libraries(test_pipeline_zerocopy ${GTEST_LIBRARIES})
add_test(test_pipeline_zerocopy ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_pipeline_zerocopy)
