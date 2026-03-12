#ifndef WFS_WRAPPER_H
#define WFS_WRAPPER_H

#include <string>
#include <memory>
#include <vector>
#include <wfslib/wfs_device.h>
#include <wfslib/file_device.h>
#include <wfslib/directory.h>
#include <wfslib/file.h>
#include <wfslib/key_file.h>

// 目录条目信息
struct DirEntry {
    std::string name;
    bool is_directory;
    uint32_t size;
};

// WFS 管理器
class WfsManager {
public:
    WfsManager() = default;
    ~WfsManager() = default;
    
    // 连接/断开
    bool Connect(const std::string& otpPath, const std::string& seepromPath, const std::string& devicePath);
    void Disconnect();
    bool IsConnected() const { return wfs_ != nullptr; }
    
    // 格式化 (基于当前设置的密钥)
    bool Format(const std::string& otpPath, const std::string& seepromPath, const std::string& devicePath);
    
    // 文件操作
    std::vector<DirEntry> ListDirectory(const std::string& path);
    bool DeleteEntry(const std::string& parentPath, const std::string& name, bool isDirectory);
    bool ImportFile(const std::string& sourcePath, const std::string& targetDir, const std::string& name);
    bool ExportFile(const std::string& sourcePath, const std::string& targetPath);
    bool CreateDirectory(const std::string& parentPath, const std::string& name);
    
    void Flush();
    
private:
    std::shared_ptr<FileDevice> device_;
    std::shared_ptr<WfsDevice> wfs_;
    std::vector<std::byte> key_;
    std::string devicePath_;
};

#endif // WFS_WRAPPER_H
