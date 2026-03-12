#include "wfs_wrapper.h"
#include <fstream>
#include <algorithm>

bool WfsManager::Connect(const std::string& otpPath, const std::string& seepromPath, const std::string& devicePath) {
    try {
        // 加载 OTP 和 SEEPROM
        auto otp = OTP::LoadFromFile(otpPath);
        if (!otp) return false;
        
        auto seeprom = SEEPROM::LoadFromFile(seepromPath);
        if (!seeprom) return false;
        
        key_ = seeprom->GetUSBKey(*otp);
        
        // 创建设备 (读写模式)
        // 计算扇区数
        std::ifstream testFile(devicePath, std::ios::binary | std::ios::ate);
        uint64_t fileSize = testFile.tellg();
        testFile.close();
        uint32_t sectorsCount = static_cast<uint32_t>(fileSize / 512);
        
        device_ = std::make_shared<FileDevice>(devicePath, 9, sectorsCount, false);
        
        // 打开 WFS
        auto result = WfsDevice::Open(device_, key_);
        if (!result) {
            device_.reset();
            return false;
        }
        wfs_ = *result;
        return true;
    } catch (...) {
        return false;
    }
}

void WfsManager::Disconnect() {
    if (wfs_) {
        wfs_->Flush();
    }
    wfs_.reset();
    device_.reset();
    key_.clear();
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
