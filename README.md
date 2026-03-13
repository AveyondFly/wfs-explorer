# WFS Explorer

A Windows GUI application for managing Wii U WFS (Wii U File System) partitions. Built with Dear ImGui and DirectX 11.

![License](https://img.shields.io/badge/license-MIT-blue.svg)

## Features

- **Browse WFS Partitions**: Navigate through directories and files on Wii U hard drive partitions
- **USB & MLC Support**: Automatic detection of USB and MLC (internal storage) device types
- **File Operations**: Import, export, and delete files from WFS partitions
- **Format Support**: Format drives as WFS partitions (USB or MLC type) using OTP/SEEPROM keys
- **Bilingual UI**: Automatic Chinese/English interface based on Windows system language
- **Safe Operations**: Protection against disconnect during file operations

## System Requirements

- Windows 10/11 (64-bit)
- Administrator privileges (required for raw disk access)

## Project Structure

```
wfs-explorer/
├── imgui/                    # Dear ImGui library
│   ├── backend/              # Win32 and DirectX 11 backends
│   └── src/                  # Application source code
│       ├── main.cpp          # Entry point, DirectX 11 setup
│       ├── app.cpp/h         # Main application logic
│       ├── wfs_wrapper.cpp/h # WFS library wrapper
│       └── i18n.cpp/h        # Internationalization
├── build-imgui.sh            # Linux cross-compilation script
└── README.md
```

### Dependencies

- [wfslib](https://github.com/koolkdev/wfslib) - Wii U WFS filesystem library
- [Dear ImGui](https://github.com/ocornut/imgui) - Immediate mode GUI library
- [Crypto++](https://cryptopp.com/) - Cryptographic library (via vcpkg)
- [Boost](https://www.boost.org/) - Boost Iostreams/Filesystem (via vcpkg)

## Building

### Prerequisites

1. Linux system with MinGW-w64 cross-compiler
2. [vcpkg](https://vcpkg.io/) with mingw triplets
3. [wfslib](https://github.com/koolkdev/wfslib) compiled for MinGW

### Install vcpkg Dependencies

```bash
# Install vcpkg (if not already installed)
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# Install dependencies for MinGW
./vcpkg install boost-iostreams:x64-mingw-static-no-hidden
./vcpkg install boost-filesystem:x64-mingw-static-no-hidden
./vcpkg install cryptopp:x64-mingw-static-no-hidden
```

### Build wfslib

```bash
git clone https://github.com/koolkdev/wfslib.git
cd wfslib

# Create MinGW build directory
mkdir build-mingw && cd build-mingw

# Configure and build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DVCPKG_TARGET_TRIPLET=x64-mingw-static-no-hidden \
    -DVCPKG_ROOT=/path/to/vcpkg

cmake --build .
```

### Build WFS Explorer

Edit `build-imgui.sh` to set the correct paths:

```bash
WFSLIB_PATH="/path/to/wfslib"
VCPKG_PATH="/path/to/vcpkg/installed/x64-mingw-static-no-hidden"
```

Then run:

```bash
./build-imgui.sh
```

The output will be `build-imgui/wfs-explorer.exe`.

## Usage

### Preparation

1. **Obtain OTP and SEEPROM files** from your Wii U console:
   - `otp.bin` - 1024 bytes, contains encryption keys (required for all devices)
   - `seeprom.bin` - 512 bytes, contains USB keys (required for USB devices, optional for MLC)
   
   These files are required to decrypt WFS partitions.

2. **Connect Wii U storage** to your Windows PC
   - USB: External hard drive with WFS-formatted partitions
   - MLC: Wii U internal storage (dumped as disk image)
   - Run the application as Administrator for raw disk access

### Connecting to a Partition

1. Launch `wfs-explorer.exe` as Administrator
2. Click `...` next to **OTP File** and select your `otp.bin`
3. (Optional for MLC) Click `...` next to **SEEPROM File** and select your `seeprom.bin`
4. Select the drive letter from the **Wii U Partition** dropdown
5. Click **Connect**

The application will automatically detect whether the partition is USB or MLC type and use the appropriate decryption key.

### File Operations

- **Navigate**: Double-click directories to enter, use **Up** button to go back
- **Import**: Click **Import** to copy files from PC to the WFS partition
- **Export**: Select a file and click **Export** to copy from WFS to PC
- **Delete**: Select a file/directory and click **Delete**
- **Refresh**: Click **Refresh** to reload the current directory

### Formatting a Partition

1. Select OTP file (required)
2. Select SEEPROM file (required for USB, not needed for MLC)
3. Select target drive
4. Click **Format**
5. Select device type:
   - **USB** - External USB hard drive (requires SEEPROM)
   - **MLC** - Wii U internal storage (no SEEPROM needed)
6. Read the warning carefully
7. Check "I understand, proceed with format"
8. Click **Format** to confirm

**Warning**: Formatting will erase ALL data on the selected partition!

## Error Messages

| Error | Description |
|-------|-------------|
| OTP file not found | The specified OTP file does not exist |
| SEEPROM file not found | The specified SEEPROM file does not exist (required for USB) |
| OTP file size invalid | OTP file must be exactly 1024 bytes |
| SEEPROM file size invalid | SEEPROM file must be exactly 512 bytes |
| Device/drive not found | The selected drive does not exist |
| Failed to open device | Run as Administrator for disk access |
| Not a WFS partition | The selected partition is not WFS formatted |
| Key mismatch | Wrong OTP/SEEPROM for this partition |

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [wfslib](https://github.com/koolkdev/wfslib) by koolkdev
- [Dear ImGui](https://github.com/ocornut/imgui) by Omar Cornut
- Wii U homebrew community
