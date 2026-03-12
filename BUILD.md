# WFS Explorer 编译说明

## Windows 编译步骤

### 1. 安装依赖
- Visual Studio 2022 (C++ 桌面开发工作负载)
- CMake
- vcpkg (可选)

### 2. 编译 wfslib
```powershell
cd wfslib
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### 3. 安装依赖库 (使用 vcpkg)
```powershell
vcpkg install cryptopp boost-iostreams boost-filesystem
```

### 4. 编译 wfs-explorer
```powershell
cd wfs-explorer
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Linux 交叉编译 (MinGW)

### 1. 安装 MinGW-w64
```bash
sudo apt install mingw-w64
```

### 2. 安装交叉编译依赖
这需要手动编译 cryptopp 和 boost 的 Windows 版本，比较复杂。

建议直接在 Windows 上编译。

## 运行

编译完成后，将以下文件放在同一目录：
- wfs-explorer.exe
- cryptlib.dll (或静态链接)
- boost 相关 DLL (或静态链接)
