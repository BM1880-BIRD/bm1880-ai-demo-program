set(LIBLINEAR_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/modules/3rd/LibLinear
    ${CMAKE_SOURCE_DIR}/modules/3rd/LibLinear/blas
)

set(LIBLINEAR_SRC
    ${CMAKE_SOURCE_DIR}/modules/3rd/LibLinear/linear.cpp
    ${CMAKE_SOURCE_DIR}/modules/3rd/LibLinear/tron.cpp
    ${CMAKE_SOURCE_DIR}/modules/3rd/LibLinear/blas/combined.c
)