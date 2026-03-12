#include "wfs_wrapper.h"
#include "i18n.h"

namespace Strings {
    const char* TranslateError(ConnectError error) {
        switch (error) {
            case ConnectError::None:
                return "";
            case ConnectError::OtpNotFound:
                return TR("OTP file not found.", "OTP 文件未找到。");
            case ConnectError::SeepromNotFound:
                return TR("SEEPROM file not found.", "SEEPROM 文件未找到。");
            case ConnectError::OtpInvalidSize:
                return TR("OTP file size invalid (expected 1024 bytes).", "OTP 文件大小无效（应为 1024 字节）。");
            case ConnectError::SeepromInvalidSize:
                return TR("SEEPROM file size invalid (expected 512 bytes).", "SEEPROM 文件大小无效（应为 512 字节）。");
            case ConnectError::DeviceNotFound:
                return TR("Device/drive not found.", "设备/驱动器未找到。");
            case ConnectError::DeviceOpenFailed:
                return TR("Failed to open device. Run as Administrator?", "无法打开设备。请以管理员身份运行？");
            case ConnectError::NotWfsPartition:
                return TR("Not a WFS partition.", "不是 WFS 分区。");
            case ConnectError::InvalidWfsVersion:
                return TR("Unsupported WFS version.", "不支持的 WFS 版本。");
            case ConnectError::KeyMismatch:
                return TR("Key mismatch. Wrong OTP/SEEPROM files?", "密钥不匹配。OTP/SEEPROM 文件错误？");
            default:
                return TR("Unknown error.", "未知错误。");
        }
    }
}
