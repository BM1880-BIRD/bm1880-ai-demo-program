set(HISIV300_TOOLCHAIN_BIN /source/bm1880/host-tools/arm-hisiv300-linux/bin/)
set(ARCH arm-hisiv300-linux-uclibcgnueabi-)
set(CMAKE_C_COMPILER ${HISIV300_TOOLCHAIN_BIN}${ARCH}gcc)
set(CMAKE_CXX_COMPILER ${HISIV300_TOOLCHAIN_BIN}${ARCH}g++)