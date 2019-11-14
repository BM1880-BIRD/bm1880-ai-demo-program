include(${CMAKE_SOURCE_DIR}/cmake/library/${PLATFORM}.cmake)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/platforms/linux/${ARCH}.toolchain.cmake)
endif()

if (NOT EXISTS ${CMAKE_TOOLCHAIN_FILE})
    message(FATAL_ERROR "cannot find toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
endif()

set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

if (DEFINED CUSTOM_PREBUILT_DIR)
    add_custom_target(prebuilt)
else()
    set(TAR_NAME "${PREBUILT_DIR}/${PLATFORM}.tar.gz")
    make_directory(${PREBUILT_DIR}/${PLATFORM})
    add_custom_target(prebuilt
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${TAR_NAME} WORKING_DIRECTORY ${PREBUILT_DIR}/${PLATFORM}
    )
endif()

if(DEFINED CUSTOM_PREBUILT_DIR)
    set(GTEST_LIBRARIES "${CUSTOM_PREBUILT_DIR}/lib/libgtest.a")
else()
    set(GTEST_LIBRARIES "${PREBUILT_DIR}/${PLATFORM}/lib/libgtest.a")
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
