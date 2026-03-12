cmake_minimum_required(VERSION 3.20)
project(wfs-explorer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Win32 GUI 设置
if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()

# wfslib 路径（手动设置）
set(WFSLIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../wfslib" CACHE PATH "wfslib directory")

# 源文件
add_executable(wfs-explorer WIN32
    src/main.cpp
    src/mainwindow.cpp
    src/wfs_wrapper.cpp
)

# 包含路径
target_include_directories(wfs-explorer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${WFSLIB_DIR}/include
    ${WFSLIB_DIR}/src
)

# 预编译库（Windows 用户需要先编译 wfslib）
if(WIN32)
    # Windows: 链接预编译的 wfslib 和依赖
    find_library(WFSLIB_LIB 
        NAMES wfslib libwfslib
        PATHS 
            ${WFSLIB_DIR}/build/Release
            ${WFSLIB_DIR}/build/Debug
            ${WFSLIB_DIR}/out/build/x64-Release
            ${WFSLIB_DIR}/out/build/x64-Debug
    )
    
    # 手动指定依赖库路径（用户需要调整）
    target_link_libraries(wfs-explorer PRIVATE
        ${WFSLIB_LIB}
        comctl32
        shell32
        ole32
        cryptlib
        boost_iostreams
        boost_filesystem
    )
else()
    # Linux: 尝试使用 pkg-config 或手动链接
    find_library(WFSLIB_LIB 
        NAMES wfslib libwfslib
        PATHS ${WFSLIB_DIR}/build/default
    )
    
    target_link_libraries(wfs-explorer PRIVATE
        ${WFSLIB_LIB}
        cryptopp
        boost_iostreams
        boost_filesystem
    )
endif()

# 安装
install(TARGETS wfs-explorer DESTINATION bin)
