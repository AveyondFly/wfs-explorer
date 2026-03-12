#!/bin/bash

WFSLIB_PATH="/home/ubuntu/code/wfslib"
VCPKG_PATH="/home/ubuntu/code/vcpkg/installed/x64-mingw-static-no-hidden"
BUILD_DIR="build-imgui"

echo "编译 WFS Explorer (ImGui)..."

mkdir -p $BUILD_DIR

CXX="x86_64-w64-mingw32-g++"
CFLAGS="-O2 -std=c++23 -static -mwindows"
INCLUDES="-I./imgui -I./imgui/backend -I${WFSLIB_PATH}/include -I${WFSLIB_PATH}/include/wfslib -I${WFSLIB_PATH}/src -I${VCPKG_PATH}/include"
LIBS="-L${WFSLIB_PATH}/build-mingw -L${VCPKG_PATH}/lib -lwfslib -lcryptopp -lboost_iostreams-gcc13-mt-x64-1_90 -lboost_filesystem-gcc13-mt-x64-1_90 -lboost_atomic-gcc13-mt-x64-1_90 -lzlib -lbz2 -llzma -lzstd -lmingw32 -lgdi32 -ld3d11 -ld3dcompiler -ldxgi -lole32 -loleaut32 -luuid -lcomdlg32 -lshell32 -ldwmapi"

IMGUI_SRCS="imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backend/imgui_impl_win32.cpp imgui/backend/imgui_impl_dx11.cpp"

APP_SRCS="imgui/src/main.cpp imgui/src/app.cpp imgui/src/wfs_wrapper.cpp"

OBJS=""
for SRC in $IMGUI_SRCS $APP_SRCS; do
    OBJ="${BUILD_DIR}/$(basename ${SRC%.cpp}).o"
    echo "编译: $SRC"
    $CXX $CFLAGS $INCLUDES -c $SRC -o $OBJ || exit 1
    OBJS="$OBJS $OBJ"
done

echo "链接..."
$CXX $CFLAGS $OBJS -o $BUILD_DIR/wfs-explorer.exe $LIBS || exit 1

echo "完成! 输出: $BUILD_DIR/wfs-explorer.exe"
ls -la $BUILD_DIR/wfs-explorer.exe
