#include "app.h"
#include "i18n.h"
#include <imgui.h>
#include <windows.h>
#include <commdlg.h>
#include <vector>
#include <algorithm>

WfsApp::WfsApp() {
    ScanDrives();
}

WfsApp::~WfsApp() {
    if (connected_) {
        wfs_.Disconnect();
    }
}

bool WfsApp::BeginOperation() {
    if (!connected_) return false;
    operationCount_++;
    return true;
}

void WfsApp::EndOperation() {
    if (operationCount_ > 0) operationCount_--;
}

void WfsApp::ScanDrives() {
    drives_.clear();
    char buffer[256];
    GetLogicalDriveStringsA(sizeof(buffer), buffer);
    char* p = buffer;
    while (*p) {
        std::string drive(p);
        if (GetDriveTypeA(drive.c_str()) == DRIVE_FIXED) {
            drives_.push_back(drive);
        }
        p += drive.length() + 1;
    }
    if (drives_.empty()) {
        drives_.push_back("C:\\");
    }
}

std::string WfsApp::GetSelectedDrivePath() {
    if (selectedDrive_ >= 0 && selectedDrive_ < (int)drives_.size()) {
        return "\\\\.\\" + drives_[selectedDrive_].substr(0, 2);
    }
    return "";
}

void WfsApp::Render() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin(Strings::Title(), nullptr, 
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    
    float panelWidth = 280.0f;
    
    ImGui::BeginChild("LeftPanel", ImVec2(panelWidth, -1), true);
    RenderConnectPanel();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    ImGui::BeginChild("RightPanel", ImVec2(-1, -1), true);
    RenderFileList();
    ImGui::EndChild();
    
    ImGui::End();
    
    RenderStatusBar();
}

void WfsApp::RenderConnectPanel() {
    ImGui::Text("%s", Strings::Title());
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", Strings::Subtitle());
    ImGui::Separator();
    
    // OTP 文件
    ImGui::Text("%s", Strings::OtpFile());
    ImGui::InputText("##otp", otpPath_, sizeof(otpPath_));
    ImGui::SameLine();
    if (ImGui::Button("...##otpbrowse")) {
        char filename[MAX_PATH] = {0};
        OPENFILENAMEA ofn = {0};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST;
        if (GetOpenFileNameA(&ofn)) {
            strncpy(otpPath_, filename, sizeof(otpPath_) - 1);
        }
    }
    
    // SEEPROM 文件 (MLC 可选)
    ImGui::Text("%s %s", Strings::SeepromFile(), Strings::SeepromOptional());
    ImGui::InputText("##seeprom", seepromPath_, sizeof(seepromPath_));
    ImGui::SameLine();
    if (ImGui::Button("...##seeprombrowse")) {
        char filename[MAX_PATH] = {0};
        OPENFILENAMEA ofn = {0};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST;
        if (GetOpenFileNameA(&ofn)) {
            strncpy(seepromPath_, filename, sizeof(seepromPath_) - 1);
        }
    }
    
    ImGui::Separator();

    // 驱动器选择
    ImGui::Text("%s", Strings::WiiUPartition());
    const char* drivePreview = (selectedDrive_ >= 0 && selectedDrive_ < (int)drives_.size())
        ? drives_[selectedDrive_].c_str() : Strings::SelectDrive();
    if (ImGui::BeginCombo("##drive", drivePreview)) {
        for (int i = 0; i < (int)drives_.size(); i++) {
            bool selected = (i == selectedDrive_);
            if (ImGui::Selectable(drives_[i].c_str(), selected)) {
                selectedDrive_ = i;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    
    ImGui::Separator();
    
    // 按钮
    if (connected_) {
        if (ImGui::Button(Strings::Disconnect(), ImVec2(-1, 30))) {
            Disconnect();
        }
    } else {
        if (ImGui::Button(Strings::Connect(), ImVec2(-1, 30))) {
            Connect();
        }
        if (ImGui::Button(Strings::Format(), ImVec2(-1, 30))) {
            Format();
        }
    }
    
    // 连接状态
    ImGui::Separator();
    if (connected_) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s", Strings::Connected());
        // 显示设备类型
        const char* deviceTypeStr = Strings::DeviceTypeUnknown();
        switch (wfs_.GetDeviceType()) {
            case WfsDeviceType::USB: deviceTypeStr = Strings::DeviceTypeUSB(); break;
            case WfsDeviceType::MLC: deviceTypeStr = Strings::DeviceTypeMLC(); break;
            default: break;
        }
        ImGui::Text("%s %s", Strings::DeviceType(), deviceTypeStr);
    } else {
        ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "%s", Strings::Disconnected());
    }
}

void WfsApp::RenderFileList() {
    // 工具栏
    if (ImGui::Button(Strings::Refresh()) && connected_) {
        RefreshList();
    }
    ImGui::SameLine();
    if (ImGui::Button(Strings::Up()) && connected_) {
        NavigateUp();
    }
    ImGui::SameLine();
    if (ImGui::Button(Strings::Import()) && connected_) {
        ImportFile();
    }
    ImGui::SameLine();
    if (ImGui::Button(Strings::Export()) && connected_ && selectedIndex_ >= 0) {
        ExportSelected();
    }
    ImGui::SameLine();
    if (ImGui::Button(Strings::Delete()) && connected_ && selectedIndex_ >= 0) {
        DeleteSelected();
    }
    
    ImGui::Separator();
    
    // 当前路径
    ImGui::Text("%s %s", Strings::Path(), currentPath_.empty() ? "\\" : currentPath_.c_str());
    
    ImGui::Separator();
    
    // 文件列表表格
    if (ImGui::BeginTable("Files", 3, 
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn(Strings::Name(), ImGuiTableColumnFlags_WidthStretch, 0.6f);
        ImGui::TableSetupColumn(Strings::Type(), ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn(Strings::Size(), ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableHeadersRow();
        
        if (connected_) {
            // 返回上级目录
            if (!currentPath_.empty() && currentPath_ != "\\") {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (ImGui::Selectable("..", false, ImGuiSelectableFlags_SpanAllColumns)) {
                    NavigateUp();
                }
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", Strings::UpDir());
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("");
            }
            
            // 文件列表
            for (int i = 0; i < (int)entries_.size(); i++) {
                const auto& entry = entries_[i];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                
                bool selected = (i == selectedIndex_);
                if (ImGui::Selectable(entry.name.c_str(), selected, 
                    ImGuiSelectableFlags_SpanAllColumns)) {
                    selectedIndex_ = i;
                }
                
                // 双击进入目录
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    if (entry.isDirectory) {
                        NavigateTo(entry);
                    }
                }
                
                // 右键菜单
                if (ImGui::BeginPopupContextItem()) {
                    if (entry.isDirectory) {
                        if (ImGui::MenuItem(Strings::Open())) NavigateTo(entry);
                    }
                    if (ImGui::MenuItem(Strings::Export())) { selectedIndex_ = i; ExportSelected(); }
                    if (ImGui::MenuItem(Strings::Delete())) { selectedIndex_ = i; DeleteSelected(); }
                    ImGui::EndPopup();
                }
                
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", entry.isDirectory ? Strings::DirType() : Strings::FileType());
                ImGui::TableSetColumnIndex(2);
                if (!entry.isDirectory) {
                    char sizeStr[32];
                    snprintf(sizeStr, sizeof(sizeStr), "%u", entry.size);
                    ImGui::Text("%s", sizeStr);
                }
            }
        }
        
        ImGui::EndTable();
    }
    
    if (!connected_) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", Strings::SelectPrompt());
    }
}

void WfsApp::RenderStatusBar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 25));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 25));
    ImGui::Begin("StatusBar", nullptr, 
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    
    if (connected_) {
        ImGui::Text("%s", Strings::FilesCount(entries_.size()));
    } else {
        ImGui::Text("%s", Strings::Disconnected());
    }
    
    ImGui::End();
}

void WfsApp::Connect() {
    // OTP 是必需的
    if (strlen(otpPath_) == 0) {
        ShowErrorDialog(Strings::ErrMissingFiles(), Strings::ErrMissingFilesMsg());
        return;
    }
    
    std::string devicePath = GetSelectedDrivePath();
    if (devicePath.empty()) {
        ShowErrorDialog(Strings::ErrNoDrive(), Strings::ErrNoDriveMsg());
        return;
    }
    
    // SEEPROM 可以为空（MLC 设备只需要 OTP）
    // wfs_wrapper 会自动尝试 USB 和 MLC 密钥
    ConnectError error = wfs_.Connect(otpPath_, seepromPath_, devicePath);
    if (error != ConnectError::None) {
        ShowErrorDialog(Strings::ErrConnectFailed(), Strings::TranslateError(error));
        return;
    }
    
    connected_ = true;
    currentPath_ = "\\";
    selectedIndex_ = -1;
    RefreshList();
}

void WfsApp::Disconnect() {
    if (operationCount_ > 0) {
        ShowErrorDialog(Strings::ErrOperationInProgress(), Strings::ErrOperationInProgressMsg());
        return;
    }
    
    wfs_.Disconnect();
    connected_ = false;
    entries_.clear();
    currentPath_ = "\\";
    selectedIndex_ = -1;
}

void WfsApp::Format() {
    // OTP 是必需的
    if (strlen(otpPath_) == 0) {
        ShowErrorDialog(Strings::ErrMissingFiles(), Strings::ErrMissingFilesMsg());
        return;
    }
    
    ImGui::OpenPopup(Strings::FormatWarning());
    if (ImGui::BeginPopupModal(Strings::FormatWarning(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", Strings::FormatWarningTitle());
        ImGui::Text("%s", Strings::FormatWarningMsg1());
        ImGui::Text("%s", Strings::FormatWarningMsg2());
        ImGui::Separator();
        
        // 设备类型选择
        ImGui::Text("%s", Strings::DeviceType());
        if (ImGui::RadioButton(Strings::DeviceTypeUSB(), formatDeviceType_ == 0)) {
            formatDeviceType_ = 0;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton(Strings::DeviceTypeMLC(), formatDeviceType_ == 1)) {
            formatDeviceType_ = 1;
        }
        
        // USB 需要 SEEPROM
        if (formatDeviceType_ == 0 && strlen(seepromPath_) == 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "%s", Strings::ErrMissingSeepromMsg());
        }
        
        ImGui::Separator();
        
        static bool confirmed = false;
        ImGui::Checkbox(Strings::FormatConfirm(), &confirmed);
        
        if (ImGui::Button(Strings::Cancel(), ImVec2(120, 0))) {
            confirmed = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        
        bool canFormat = confirmed && (formatDeviceType_ == 1 || strlen(seepromPath_) > 0);
        if (canFormat) {
            if (ImGui::Button(Strings::Format(), ImVec2(120, 0))) {
                std::string devicePath = GetSelectedDrivePath();
                WfsDeviceType deviceType = (formatDeviceType_ == 0) ? WfsDeviceType::USB : WfsDeviceType::MLC;
                ConnectError error = wfs_.Format(otpPath_, seepromPath_, devicePath, deviceType);
                
                if (error != ConnectError::None) {
                    ShowErrorDialog(Strings::FormatFailed(), Strings::TranslateError(error));
                } else {
                    connected_ = true;
                    currentPath_ = "\\";
                    selectedIndex_ = -1;
                    RefreshList();
                }
                confirmed = false;
                ImGui::CloseCurrentPopup();
            }
        } else {
            ImGui::BeginDisabled();
            ImGui::Button(Strings::Format(), ImVec2(120, 0));
            ImGui::EndDisabled();
        }
        
        ImGui::EndPopup();
    }
}

void WfsApp::RefreshList() {
    if (!connected_) return;
    
    entries_.clear();
    auto dirEntries = wfs_.ListDirectory(currentPath_);
    
    std::vector<FileEntry> dirs, files;
    for (const auto& e : dirEntries) {
        if (e.is_directory) dirs.push_back({e.name, true, 0});
        else files.push_back({e.name, false, e.size});
    }
    
    auto cmp = [](const FileEntry& a, const FileEntry& b) { return a.name < b.name; };
    std::sort(dirs.begin(), dirs.end(), cmp);
    std::sort(files.begin(), files.end(), cmp);
    
    entries_ = dirs;
    entries_.insert(entries_.end(), files.begin(), files.end());
}

void WfsApp::NavigateUp() {
    if (currentPath_ == "\\") return;
    size_t pos = currentPath_.find_last_of('\\');
    if (pos == 0) currentPath_ = "\\";
    else currentPath_ = currentPath_.substr(0, pos);
    selectedIndex_ = -1;
    RefreshList();
}

void WfsApp::NavigateTo(const FileEntry& entry) {
    if (currentPath_ == "\\") {
        currentPath_ = "\\" + entry.name;
    } else {
        currentPath_ = currentPath_ + "\\" + entry.name;
    }
    selectedIndex_ = -1;
    RefreshList();
}

void WfsApp::DeleteSelected() {
    if (selectedIndex_ < 0 || selectedIndex_ >= (int)entries_.size()) return;
    
    const auto& entry = entries_[selectedIndex_];
    if (entry.name == "." || entry.name == "..") return;
    
    std::string msg = entry.isDirectory ?
        std::string(Strings::DeleteFolderMsg()) + entry.name :
        std::string(Strings::DeleteFileMsg()) + entry.name;
    
    ImGui::OpenPopup(Strings::ConfirmDelete());
    if (ImGui::BeginPopupModal(Strings::ConfirmDelete(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", msg.c_str());
        ImGui::Separator();
        
        if (ImGui::Button(Strings::Cancel(), ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(Strings::Delete(), ImVec2(120, 0))) {
            if (!BeginOperation()) {
                ImGui::CloseCurrentPopup();
                return;
            }
            
            bool success = wfs_.DeleteEntry(currentPath_, entry.name, entry.isDirectory);
            
            EndOperation();
            
            if (success) {
                wfs_.Flush();
                selectedIndex_ = -1;
                RefreshList();
            } else {
                ShowErrorDialog(Strings::ErrDeleteFailed(), Strings::ErrDeleteFailedMsg());
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void WfsApp::ImportFile() {
    if (!connected_) return;
    
    char filename[MAX_PATH] = {0};
    OPENFILENAMEA ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;
    
    if (GetOpenFileNameA(&ofn)) {
        std::string filepath = filename;
        std::string name = filepath;
        size_t pos = filepath.find_last_of("\\/");
        if (pos != std::string::npos) name = filepath.substr(pos + 1);
        
        if (!BeginOperation()) return;
        
        bool success = wfs_.ImportFile(filepath, currentPath_, name);
        
        EndOperation();
        
        if (success) {
            wfs_.Flush();
            RefreshList();
        } else {
            ShowErrorDialog(Strings::ErrImportFailed(), Strings::ErrImportFailedMsg());
        }
    }
}

void WfsApp::ExportSelected() {
    if (selectedIndex_ < 0 || selectedIndex_ >= (int)entries_.size()) return;
    
    const auto& entry = entries_[selectedIndex_];
    if (entry.isDirectory) return;
    
    char filename[MAX_PATH] = {0};
    strncpy(filename, entry.name.c_str(), MAX_PATH - 1);
    
    OPENFILENAMEA ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    
    if (GetSaveFileNameA(&ofn)) {
        std::string sourcePath = currentPath_ == "\\" ?
            "\\" + entry.name : currentPath_ + "\\" + entry.name;
        
        if (!BeginOperation()) return;
        
        bool success = wfs_.ExportFile(sourcePath, filename);
        
        EndOperation();
        
        if (!success) {
            ShowErrorDialog(Strings::ErrExportFailed(), Strings::ErrExportFailedMsg());
        }
    }
}

void WfsApp::ShowErrorDialog(const std::string& title, const std::string& message) {
    ImGui::OpenPopup(title.c_str());
    if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", message.c_str());
        ImGui::Separator();
        if (ImGui::Button(Strings::OK(), ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}