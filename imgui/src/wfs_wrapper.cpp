#include "wfs_wrapper.h"
#include <fstream>
#include <algorithm>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>
// Undefine Windows macros that conflict with method names
#ifdef DeleteFile
#undef DeleteFile
#endif
#ifdef CreateFile
#undef CreateFile
#endif
#ifdef CreateDirectory
#undef CreateDirectory
#endif
#endif

// Store last error details for diagnostics
static std::string g_lastErrorDetail;

static bool file_exists(const std::string& path) {
#ifdef _WIN32
    // On Windows, check if it's a device path (\\.\X:)
    if (path.length() >= 4 && path.substr(0, 4) == "\\\\.\\") {
        // Try to open the device to check if it exists
        HANDLE hDevice = CreateFileA(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        if (hDevice != INVALID_HANDLE_VALUE) {
            CloseHandle(hDevice);
            return true;
        }
        return false;
    }
#endif
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

static size_t file_size(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        return buffer.st_size;
    }
    return 0;
}

std::string WfsManager::GetErrorDescription(ConnectError error) {
    switch (error) {
        case ConnectError::None:
            return "Success";
        case ConnectError::OtpNotFound:
            return "OTP file not found";
        case ConnectError::SeepromNotFound:
            return "SEEPROM file not found";
        case ConnectError::OtpInvalidSize:
            return "Invalid OTP file size (expected 1024 bytes)";
        case ConnectError::SeepromInvalidSize:
            return "Invalid SEEPROM file size (expected 512 bytes)";
        case ConnectError::DeviceNotFound:
            return "Device/partition not found or inaccessible";
        case ConnectError::DeviceOpenFailed:
            return "Failed to open device: " + g_lastErrorDetail;
        case ConnectError::NotWfsPartition:
            return "Not a Wii U WFS partition (invalid header)";
        case ConnectError::InvalidWfsVersion:
            return "Invalid WFS version (partition may be corrupted)";
        case ConnectError::KeyMismatch:
            return "Key mismatch (wrong OTP/SEEPROM for this partition)";
        case ConnectError::UnknownError:
        default:
            return "Unknown error";
    }
}

ConnectError WfsManager::Connect(const std::string& otpPath, const std::string& seepromPath, const std::string& devicePath) {
    Disconnect();
    
    // 检查 OTP 文件
    if (!file_exists(otpPath)) {
        return ConnectError::OtpNotFound;
    }
    if (file_size(otpPath) != OTP::OTP_SIZE) {
        return ConnectError::OtpInvalidSize;
    }
    
    // 检查 SEEPROM 文件 (USB 需要)
    bool hasSeeprom = file_exists(seepromPath);
    if (hasSeeprom && file_size(seepromPath) != SEEPROM::SEEPROM_SIZE) {
        return ConnectError::SeepromInvalidSize;
    }
    
    // 检查设备
    if (!file_exists(devicePath)) {
        return ConnectError::DeviceNotFound;
    }
    
    try {
        // 加载 OTP
        auto otp = OTP::LoadFromFile(otpPath);
        if (!otp) {
            return ConnectError::OtpNotFound;
        }
        
        devicePath_ = devicePath;
        
        // 获取设备大小
        uint64_t fileSize = 0;
#ifdef _WIN32
        // On Windows, use CreateFile to open device and get size
        HANDLE hDevice = CreateFileA(
            devicePath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        if (hDevice == INVALID_HANDLE_VALUE) {
            DWORD err = GetLastError();
            g_lastErrorDetail = "Windows error code: " + std::to_string(err);
            if (err == 5) {
                g_lastErrorDetail += " (Access denied - need Administrator)";
            } else if (err == 2) {
                g_lastErrorDetail += " (File not found)";
            } else if (err == 32) {
                g_lastErrorDetail += " (File in use by another process)";
            }
            return ConnectError::DeviceOpenFailed;
        }
        
        // Use DeviceIoControl to get device size (GetFileSizeEx doesn't work for devices)
        GET_LENGTH_INFORMATION lengthInfo;
        DWORD bytesReturned;
        if (DeviceIoControl(hDevice, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, 
                           &lengthInfo, sizeof(lengthInfo), &bytesReturned, NULL)) {
            fileSize = lengthInfo.Length.QuadPart;
        } else {
            // Fallback: try GetFileSizeEx
            LARGE_INTEGER size;
            if (GetFileSizeEx(hDevice, &size)) {
                fileSize = size.QuadPart;
            } else {
                DWORD err = GetLastError();
                g_lastErrorDetail = "Cannot determine device size, error: " + std::to_string(err);
                CloseHandle(hDevice);
                return ConnectError::DeviceOpenFailed;
            }
        }
        CloseHandle(hDevice);
#else
        // On Linux, use stat
        struct stat st;
        if (stat(devicePath.c_str(), &st) != 0) {
            return ConnectError::DeviceOpenFailed;
        }
        fileSize = st.st_size;
#endif
        
        uint32_t sectorsCount = static_cast<uint32_t>(fileSize / 512);
        
        device_ = std::make_shared<FileDevice>(devicePath, 9, sectorsCount, false);
        
        // 准备两种密钥尝试
        std::vector<std::vector<std::byte>> keysToTry;
        std::vector<WfsDeviceType> keyTypes;
        
        // 如果有 SEEPROM，先尝试 USB 密钥
        if (hasSeeprom) {
            auto seeprom = SEEPROM::LoadFromFile(seepromPath);
            if (seeprom) {
                keysToTry.push_back(seeprom->GetUSBKey(*otp));
                keyTypes.push_back(WfsDeviceType::USB);
            }
        }
        
        // 再尝试 MLC 密钥
        keysToTry.push_back(otp->GetMLCKey());
        keyTypes.push_back(WfsDeviceType::MLC);
        
        // 尝试每种密钥
        WfsError lastError = WfsError::kAreaHeaderCorrupted;
        for (size_t i = 0; i < keysToTry.size(); ++i) {
            key_ = keysToTry[i];
            auto result = WfsDevice::Open(device_, key_);
            if (result) {
                wfs_ = *result;
                deviceType_ = keyTypes[i];
                return ConnectError::None;
            }
            lastError = result.error();
        }
        
        // 所有密钥都失败
        device_.reset();
        key_.clear();
        switch (lastError) {
            case WfsError::kAreaHeaderCorrupted:
                return ConnectError::NotWfsPartition;
            case WfsError::kInvalidWfsVersion:
                return ConnectError::InvalidWfsVersion;
            case WfsError::kBlockBadHash:
                return ConnectError::KeyMismatch;
            default:
                return ConnectError::UnknownError;
        }
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        if (msg.find("key file size") != std::string::npos) {
            if (msg.find("OTP") != std::string::npos || msg.find("1024") != std::string::npos) {
                return ConnectError::OtpInvalidSize;
            }
            return ConnectError::SeepromInvalidSize;
        }
        return ConnectError::UnknownError;
    } catch (...) {
        return ConnectError::UnknownError;
    }
}

ConnectError WfsManager::Format(const std::string& otpPath, const std::string& seepromPath, const std::string& devicePath, WfsDeviceType deviceType) {
    Disconnect();
    
    // 检查 OTP 文件
    if (!file_exists(otpPath)) {
        return ConnectError::OtpNotFound;
    }
    if (file_size(otpPath) != OTP::OTP_SIZE) {
        return ConnectError::OtpInvalidSize;
    }
    
    // USB 需要 SEEPROM
    if (deviceType == WfsDeviceType::USB) {
        if (!file_exists(seepromPath)) {
            return ConnectError::SeepromNotFound;
        }
        if (file_size(seepromPath) != SEEPROM::SEEPROM_SIZE) {
            return ConnectError::SeepromInvalidSize;
        }
    }
    
    // 检查设备
    if (!file_exists(devicePath)) {
        return ConnectError::DeviceNotFound;
    }
    
    try {
        // 加载 OTP
        auto otp = OTP::LoadFromFile(otpPath);
        if (!otp) {
            return ConnectError::OtpNotFound;
        }
        
        // 根据设备类型选择密钥
        DeviceType wfsDeviceType;
        if (deviceType == WfsDeviceType::USB) {
            auto seeprom = SEEPROM::LoadFromFile(seepromPath);
            if (!seeprom) {
                return ConnectError::SeepromNotFound;
            }
            key_ = seeprom->GetUSBKey(*otp);
            wfsDeviceType = DeviceType::USB;
        } else {
            key_ = otp->GetMLCKey();
            wfsDeviceType = DeviceType::MLC;
        }
        
        devicePath_ = devicePath;
        deviceType_ = deviceType;
        
        // 获取设备大小
        uint64_t fileSize = 0;
#ifdef _WIN32
        // On Windows, use CreateFile to open device and get size
        HANDLE hDevice = CreateFileA(
            devicePath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        if (hDevice == INVALID_HANDLE_VALUE) {
            return ConnectError::DeviceOpenFailed;
        }
        
        // Use DeviceIoControl to get device size (GetFileSizeEx doesn't work for devices)
        GET_LENGTH_INFORMATION lengthInfo;
        DWORD bytesReturned;
        if (DeviceIoControl(hDevice, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, 
                           &lengthInfo, sizeof(lengthInfo), &bytesReturned, NULL)) {
            fileSize = lengthInfo.Length.QuadPart;
        } else {
            CloseHandle(hDevice);
            return ConnectError::DeviceOpenFailed;
        }
        CloseHandle(hDevice);
#else
        // On Linux, use stat
        struct stat st;
        if (stat(devicePath.c_str(), &st) != 0) {
            return ConnectError::DeviceOpenFailed;
        }
        fileSize = st.st_size;
#endif
        
        uint32_t sectorsCount = static_cast<uint32_t>(fileSize / 512);
        
        device_ = std::make_shared<FileDevice>(devicePath, 9, sectorsCount, false);
        
        // 创建新的 WFS (格式化)
        auto result = WfsDevice::Create(device_, wfsDeviceType, key_);
        if (!result) {
            device_.reset();
            return ConnectError::UnknownError;
        }
        wfs_ = *result;
        wfs_->Flush();
        return ConnectError::None;
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        if (msg.find("key file size") != std::string::npos) {
            if (msg.find("OTP") != std::string::npos || msg.find("1024") != std::string::npos) {
                return ConnectError::OtpInvalidSize;
            }
            return ConnectError::SeepromInvalidSize;
        }
        return ConnectError::UnknownError;
    } catch (...) {
        return ConnectError::UnknownError;
    }
}

void WfsManager::Disconnect() {
    if (wfs_) {
        wfs_->Flush();
    }
    wfs_.reset();
    device_.reset();
    key_.clear();
    devicePath_.clear();
}

std::vector<DirEntry> WfsManager::ListDirectory(const std::string& path) {
    std::vector<DirEntry> entries;
    if (!wfs_) return entries;
    
    try {
        auto dir = wfs_->GetDirectory(path);
        if (!dir) return entries;
        
        for (const auto& item : *dir) {
            if (item.entry.has_value()) {
                DirEntry entry;
                entry.name = item.name;
                auto e = item.entry.value();
                entry.is_directory = e->is_directory();
                if (!entry.is_directory && e->is_file()) {
                    auto file = std::dynamic_pointer_cast<File>(e);
                    if (file) {
                        entry.size = file->Size();
                    }
                } else {
                    entry.size = 0;
                }
                entries.push_back(entry);
            }
        }
    } catch (...) {}
    
    return entries;
}

bool WfsManager::DeleteEntry(const std::string& parentPath, const std::string& name, bool isDirectory) {
    if (!wfs_) return false;
    
    try {
        auto dir = wfs_->GetDirectory(parentPath);
        if (!dir) return false;
        
        if (isDirectory) {
            auto result = dir->DeleteDirectory(name, true);
            return result.has_value();
        } else {
            auto result = dir->DeleteFile(name);
            return result.has_value();
        }
    } catch (...) {
        return false;
    }
}

bool WfsManager::ImportFile(const std::string& sourcePath, const std::string& targetDir, const std::string& name) {
    if (!wfs_) return false;
    
    try {
        auto dir = wfs_->GetDirectory(targetDir);
        if (!dir) return false;
        
        // 读取源文件
        std::ifstream file(sourcePath, std::ios::binary | std::ios::ate);
        size_t fileSize = file.tellg();
        file.seekg(0);
        
        std::vector<std::byte> data(fileSize);
        file.read(reinterpret_cast<char*>(data.data()), fileSize);
        
        auto result = dir->CreateFile(name, data);
        return result.has_value();
    } catch (...) {
        return false;
    }
}

bool WfsManager::ExportFile(const std::string& sourcePath, const std::string& targetPath) {
    if (!wfs_) return false;
    
    try {
        auto wfsFile = wfs_->GetFile(sourcePath);
        if (!wfsFile) return false;
        
        File::stream stream(wfsFile);
        
        std::ofstream outFile(targetPath, std::ios::binary);
        std::vector<char> buffer(0x10000);
        
        while (stream) {
            stream.read(buffer.data(), buffer.size());
            outFile.write(buffer.data(), stream.gcount());
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

bool WfsManager::CreateDirectory(const std::string& parentPath, const std::string& name) {
    if (!wfs_) return false;
    
    try {
        auto dir = wfs_->GetDirectory(parentPath);
        if (!dir) return false;
        
        auto result = dir->CreateDirectory(name);
        return result.has_value();
    } catch (...) {
        return false;
    }
}

void WfsManager::Flush() {
    if (wfs_) {
        wfs_->Flush();
    }
}
