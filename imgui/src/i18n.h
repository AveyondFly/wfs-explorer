#ifndef I18N_H
#define I18N_H

#include <string>
#include <windows.h>

// 前向声明
enum class ConnectError;

// 简单的国际化支持 - 中文/英文
class I18N {
public:
    enum Language { EN, ZH };
    
    static Language DetectLanguage() {
        LANGID langId = GetUserDefaultUILanguage();
        // 中文: 0x0804 (简体), 0x0404 (繁体)
        if (PRIMARYLANGID(langId) == LANG_CHINESE) {
            return ZH;
        }
        return EN;
    }
    
    // 翻译字符串
    static const char* T(const char* en, const char* zh) {
        static Language lang = DetectLanguage();
        return (lang == ZH) ? zh : en;
    }
};

// 便捷宏
#define TR(en, zh) I18N::T(en, zh)

// 所有 UI 字符串
namespace Strings {
    // 标题
    inline const char* Title() { return TR("WFS Explorer", "WFS 资源管理器"); }
    inline const char* Subtitle() { return TR("Wii U Partition Manager", "Wii U 分区管理器"); }
    
    // 连接面板
    inline const char* OtpFile() { return TR("OTP File:", "OTP 文件:"); }
    inline const char* SeepromFile() { return TR("SEEPROM File:", "SEEPROM 文件:"); }
    inline const char* SeepromOptional() { return TR("(Optional for MLC)", "(MLC 可选)"); }
    inline const char* WiiUPartition() { return TR("Wii U Partition:", "Wii U 分区:"); }
    inline const char* SelectDrive() { return TR("Select drive...", "选择驱动器..."); }
    inline const char* Connect() { return TR("Connect", "连接"); }
    inline const char* Disconnect() { return TR("Disconnect", "断开"); }
    inline const char* Format() { return TR("Format", "格式化"); }
    inline const char* Connected() { return TR("Connected", "已连接"); }
    inline const char* Disconnected() { return TR("Disconnected", "未连接"); }
    
    // 设备类型
    inline const char* DeviceType() { return TR("Device Type:", "设备类型:"); }
    inline const char* DeviceTypeUSB() { return TR("USB", "USB"); }
    inline const char* DeviceTypeMLC() { return TR("MLC (Internal)", "MLC (内置存储)"); }
    inline const char* DeviceTypeUnknown() { return TR("Unknown", "未知"); }
    
    // 文件列表
    inline const char* Refresh() { return TR("Refresh", "刷新"); }
    inline const char* Up() { return TR("Up", "上级"); }
    inline const char* Import() { return TR("Import", "导入"); }
    inline const char* Export() { return TR("Export", "导出"); }
    inline const char* Delete() { return TR("Delete", "删除"); }
    inline const char* Path() { return TR("Path:", "路径:"); }
    inline const char* Name() { return TR("Name", "名称"); }
    inline const char* Type() { return TR("Type", "类型"); }
    inline const char* Size() { return TR("Size", "大小"); }
    inline const char* FileType() { return TR("File", "文件"); }
    inline const char* DirType() { return TR("<DIR>", "<目录>"); }
    inline const char* UpDir() { return TR("<UP>", "<上级>"); }
    inline const char* SelectPrompt() { return TR("Select OTP, SEEPROM and drive, then click Connect", "选择 OTP、SEEPROM 和驱动器，然后点击连接"); }
    inline const char* FilesCount(size_t n) { 
        static char buf[64]; 
        snprintf(buf, sizeof(buf), TR("Connected | %zu files", "已连接 | %zu 个文件"), n);
        return buf;
    }
    
    // 右键菜单
    inline const char* Open() { return TR("Open", "打开"); }
    
    // 错误对话框
    inline const char* OK() { return TR("OK", "确定"); }
    inline const char* Cancel() { return TR("Cancel", "取消"); }
    
    // 错误标题
    inline const char* ErrMissingFiles() { return TR("Missing Files", "缺少文件"); }
    inline const char* ErrMissingFilesMsg() { return TR("Please select OTP file.", "请选择 OTP 文件。"); }
    inline const char* ErrMissingSeepromMsg() { return TR("SEEPROM is required for USB devices.", "USB 设备需要 SEEPROM 文件。"); }
    inline const char* ErrNoDrive() { return TR("No Drive", "未选择驱动器"); }
    inline const char* ErrNoDriveMsg() { return TR("Please select a drive.", "请选择一个驱动器。"); }
    inline const char* ErrConnectFailed() { return TR("Connection Failed", "连接失败"); }
    inline const char* ErrOperationInProgress() { return TR("Operation In Progress", "操作进行中"); }
    inline const char* ErrOperationInProgressMsg() { return TR("Cannot disconnect while operations are in progress.", "操作进行中无法断开连接。"); }
    inline const char* ErrDeleteFailed() { return TR("Delete Failed", "删除失败"); }
    inline const char* ErrDeleteFailedMsg() { return TR("Could not delete the selected item.", "无法删除选中的项目。"); }
    inline const char* ErrImportFailed() { return TR("Import Failed", "导入失败"); }
    inline const char* ErrImportFailedMsg() { return TR("Could not import the file.", "无法导入文件。"); }
    inline const char* ErrExportFailed() { return TR("Export Failed", "导出失败"); }
    inline const char* ErrExportFailedMsg() { return TR("Could not export the file.", "无法导出文件。"); }
    
    // 格式化对话框
    inline const char* FormatWarning() { return TR("Format Warning", "格式化警告"); }
    inline const char* FormatWarningTitle() { return TR("!!! WARNING !!!", "!!! 警告 !!!"); }
    inline const char* FormatWarningMsg1() { return TR("Formatting will ERASE ALL DATA!", "格式化将删除所有数据！"); }
    inline const char* FormatWarningMsg2() { return TR("This action cannot be undone!", "此操作无法撤销！"); }
    inline const char* FormatConfirm() { return TR("I understand, proceed with format", "我了解风险，继续格式化"); }
    inline const char* FormatFailed() { return TR("Format Failed", "格式化失败"); }
    
    // 删除确认
    inline const char* ConfirmDelete() { return TR("Confirm Delete", "确认删除"); }
    inline const char* DeleteFolderMsg() { return TR("Delete folder and all contents?\n\n", "删除文件夹及其所有内容？\n\n"); }
    inline const char* DeleteFileMsg() { return TR("Delete file?\n\n", "删除文件？\n\n"); }
    
    // 错误描述翻译
    const char* TranslateError(ConnectError error);
}

#endif
