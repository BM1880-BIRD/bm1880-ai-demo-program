function(plc_generate NAME)
    make_directory(${CMAKE_SOURCE_DIR}/src/${NAME})
    execute_process(COMMAND /usr/bin/env python3 ${CMAKE_SOURCE_DIR}/tools/plc/plc.py
                    --output-path ${CMAKE_SOURCE_DIR}/src/${NAME}
                    --header-path ${CMAKE_SOURCE_DIR}/modules/
                    ${CMAKE_SOURCE_DIR}/pipelines/${NAME}.pl
                    RESULT_VARIABLE GEN_RESULT)
    if (NOT (${GEN_RESULT} STREQUAL "0"))
        message(FATAL_ERROR "generating program ${NAME} failed")
    endif()
    execute_process(COMMAND cp -n ${CMAKE_SOURCE_DIR}/pipelines/cmake_template.txt ${CMAKE_SOURCE_DIR}/src/${NAME}/CMakeLists.txt)
endfunction()