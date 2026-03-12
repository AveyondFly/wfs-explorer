#ifndef WFS_APP_H
#define WFS_APP_H

#include <string>
#include <vector>
#include "wfs_wrapper.h"

struct FileEntry {
    std::string name;
    bool isDirectory;
    uint32_t size;
};

class WfsApp {
public:
    WfsApp();
    ~WfsApp();
    
    void Render();
    
private:
    WfsManager wfs_;
    bool connected_ = false;
    
    char otpPath_[512] = "";
    char seepromPath_[512] = "";
    int selectedDrive_ = 0;
    std::vector<std::string> drives_;
    
    std::string currentPath_;
    std::vector<FileEntry> entries_;
    int selectedIndex_ = -1;
    
    int operationCount_ = 0;
    bool BeginOperation();
    void EndOperation();
    
    void RenderConnectPanel();
    void RenderFileList();
    void RenderStatusBar();
    void ShowErrorDialog(const std::string& title, const std::string& message);
    
    void Connect();
    void Disconnect();
    void Format();
    void RefreshList();
    void NavigateUp();
    void NavigateTo(const FileEntry& entry);
    void DeleteSelected();
    void ImportFile();
    void ExportSelected();
    
    std::string GetSelectedDrivePath();
    void ScanDrives();
};

#endif
