#!/bin/bash
set -e

MINGW_GXX=x86_64-w64-mingw32-g++
WFSLIB_DIR=../wfslib
VCPKG_DIR=../vcpkg/installed/x64-mingw-static-no-hidden

INCLUDES="-I./ -I${WFSLIB_DIR}/include/wfslib -I${WFSLIB_DIR}/include -I${WFSLIB_DIR}/src -I${VCPKG_DIR}/include"
CXXFLAGS="-std=c++23 -O2 -DNOMINMAX -DWIN32_LEAN_AND_MEAN -DCRYPTOPP_DISABLE_ASM"

rm -rf build-mingw
mkdir -p build-mingw

echo "编译 wfs-explorer..."
for src in src/main.cpp src/mainwindow.cpp src/wfs_wrapper.cpp; do
    obj="build-mingw/$(basename ${src%.cpp}.o)"
    echo "编译: $src"
    ${MINGW_GXX} ${CXXFLAGS} ${INCLUDES} -c $src -o $obj
done

echo "链接..."
${MINGW_GXX} -static -mwindows \
    build-mingw/main.o build-mingw/mainwindow.o build-mingw/wfs_wrapper.o \
    -Wl,--start-group \
    ${WFSLIB_DIR}/build-mingw/libwfslib.a \
    ${VCPKG_DIR}/lib/libcryptopp.a \
    ${VCPKG_DIR}/lib/libboost_iostreams-gcc13-mt-x64-1_90.a \
    ${VCPKG_DIR}/lib/libboost_filesystem-gcc13-mt-x64-1_90.a \
    ${VCPKG_DIR}/lib/libboost_atomic-gcc13-mt-x64-1_90.a \
    ${VCPKG_DIR}/lib/libzlib.a \
    ${VCPKG_DIR}/lib/liblzma.a \
    ${VCPKG_DIR}/lib/libzstd.a \
    ${VCPKG_DIR}/lib/libbz2.a \
    -Wl,--end-group \
    -lws2_32 -lcomctl32 -lshell32 -lole32 -lgdi32 -luser32 -lkernel32 \
    -o build-mingw/wfs-explorer.exe

echo "完成! 输出: build-mingw/wfs-explorer.exe"
ls -la build-mingw/wfs-explorer.exe
