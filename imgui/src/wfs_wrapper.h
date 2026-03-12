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

// 连接错误类型
enum class ConnectError {
    None,
    OtpNotFound,
    SeepromNotFound,
    OtpInvalidSize,      // OTP 文件大小不对 (应该是 1024 字节)
    SeepromInvalidSize,  // SEEPROM 文件大小不对 (应该是 512 字节)
    DeviceNotFound,
    DeviceOpenFailed,
    NotWfsPartition,     // 不是 WFS 分区
    InvalidWfsVersion,   // WFS 版本不对
    KeyMismatch,         // 密钥不匹配
    UnknownError
};

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
    
    // 连接/断开 - 返回具体错误
    ConnectError Connect(const std::string& otpPath, const std::string& seepromPath, const std::string& devicePath);
    void Disconnect();
    bool IsConnected() const { return wfs_ != nullptr; }
    
    // 格式化 - 返回具体错误
    ConnectError Format(const std::string& otpPath, const std::string& seepromPath, const std::string& devicePath);
    
    // 获取错误描述
    static std::string GetErrorDescription(ConnectError error);
    
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
