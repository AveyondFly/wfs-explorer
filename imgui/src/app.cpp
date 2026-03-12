#include "app.h"
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
    // 主窗口
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("WFS Explorer", nullptr, 
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    
    // 分割布局
    float panelWidth = 280.0f;
    
    // 左侧面板 - 连接配置
    ImGui::BeginChild("LeftPanel", ImVec2(panelWidth, -1), true);
    RenderConnectPanel();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // 右侧面板 - 文件列表
    ImGui::BeginChild("RightPanel", ImVec2(-1, -1), true);
    RenderFileList();
    ImGui::EndChild();
    
    ImGui::End();
    
    // 状态栏
    RenderStatusBar();
}

void WfsApp::RenderConnectPanel() {
    ImGui::Text("WFS Explorer");
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Wii U Partition Manager");
    ImGui::Separator();
    
    // OTP 文件
    ImGui::Text("OTP File:");
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
    
    // SEEPROM 文件
    ImGui::Text("SEEPROM File:");
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
    ImGui::Text("Wii U Partition:");
    const char* drivePreview = (selectedDrive_ >= 0 && selectedDrive_ < (int)drives_.size())
        ? drives_[selectedDrive_].c_str() : "Select drive...";
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
        if (ImGui::Button("Disconnect", ImVec2(-1, 30))) {
            Disconnect();
        }
    } else {
        if (ImGui::Button("Connect", ImVec2(-1, 30))) {
            Connect();
        }
        if (ImGui::Button("Format", ImVec2(-1, 30))) {
            Format();
        }
    }
    
    // 连接状态
    ImGui::Separator();
    if (connected_) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Connected");
    } else {
        ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Disconnected");
    }
}

void WfsApp::RenderFileList() {
    // 工具栏
    if (ImGui::Button("Refresh") && connected_) {
        RefreshList();
    }
    ImGui::SameLine();
    if (ImGui::Button("Up") && connected_) {
        NavigateUp();
    }
    ImGui::SameLine();
    if (ImGui::Button("Import") && connected_) {
        ImportFile();
    }
    ImGui::SameLine();
    if (ImGui::Button("Export") && connected_ && selectedIndex_ >= 0) {
        ExportSelected();
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete") && connected_ && selectedIndex_ >= 0) {
        DeleteSelected();
    }
    
    ImGui::Separator();
    
    // 当前路径
    ImGui::Text("Path: %s", currentPath_.empty() ? "\\" : currentPath_.c_str());
    
    ImGui::Separator();
    
    // 文件列表表格
    if (ImGui::BeginTable("Files", 3, 
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.6f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80);
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
                ImGui::Text("<UP>");
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
                        if (ImGui::MenuItem("Open")) NavigateTo(entry);
                    }
                    if (ImGui::MenuItem("Export")) { selectedIndex_ = i; ExportSelected(); }
                    if (ImGui::MenuItem("Delete")) { selectedIndex_ = i; DeleteSelected(); }
                    ImGui::EndPopup();
                }
                
                ImGui::TableSetColumnIndex(1);
                ImGui::Text(entry.isDirectory ? "<DIR>" : "File");
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
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), 
            "Select OTP, SEEPROM and drive, then click Connect");
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
        ImGui::Text("Connected | %zu files", entries_.size());
    } else {
        ImGui::Text("Disconnected");
    }
    
    ImGui::End();
}

void WfsApp::Connect() {
    if (strlen(otpPath_) == 0 || strlen(seepromPath_) == 0) {
        ShowErrorDialog("Missing Files", "Please select OTP and SEEPROM files.");
        return;
    }
    
    std::string devicePath = GetSelectedDrivePath();
    if (devicePath.empty()) {
        ShowErrorDialog("No Drive", "Please select a drive.");
        return;
    }
    
    ConnectError error = wfs_.Connect(otpPath_, seepromPath_, devicePath);
    if (error != ConnectError::None) {
        ShowErrorDialog("Connection Failed", WfsManager::GetErrorDescription(error));
        return;
    }
    
    connected_ = true;
    currentPath_ = "\\";
    selectedIndex_ = -1;
    RefreshList();
}

void WfsApp::Disconnect() {
    if (operationCount_ > 0) {
        ShowErrorDialog("Operation In Progress", 
            "Cannot disconnect while operations are in progress.");
        return;
    }
    
    wfs_.Disconnect();
    connected_ = false;
    entries_.clear();
    currentPath_ = "\\";
    selectedIndex_ = -1;
}

void WfsApp::Format() {
    if (strlen(otpPath_) == 0 || strlen(seepromPath_) == 0) {
        ShowErrorDialog("Missing Files", "Please select OTP and SEEPROM files.");
        return;
    }
    
    // 警告对话框
    ImGui::OpenPopup("Format Warning");
    if (ImGui::BeginPopupModal("Format Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "!!! WARNING !!!");
        ImGui::Text("Formatting will ERASE ALL DATA!");
        ImGui::Text("This action cannot be undone!");
        ImGui::Separator();
        
        static bool confirmed = false;
        ImGui::Checkbox("I understand, proceed with format", &confirmed);
        
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            confirmed = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        
        if (confirmed) {
            if (ImGui::Button("Format", ImVec2(120, 0))) {
                std::string devicePath = GetSelectedDrivePath();
                ConnectError error = wfs_.Format(otpPath_, seepromPath_, devicePath);
                
                if (error != ConnectError::None) {
                    ShowErrorDialog("Format Failed", WfsManager::GetErrorDescription(error));
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
            ImGui::Button("Format", ImVec2(120, 0));
            ImGui::EndDisabled();
        }
        
        ImGui::EndPopup();
    }
}

void WfsApp::RefreshList() {
    if (!connected_) return;
    
    entries_.clear();
    auto dirEntries = wfs_.ListDirectory(currentPath_);
    
    // 分离目录和文件
    std::vector<FileEntry> dirs, files;
    for (const auto& e : dirEntries) {
        if (e.is_directory) dirs.push_back({e.name, true, 0});
        else files.push_back({e.name, false, e.size});
    }
    
    // 先目录后文件，按名称排序
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
    
    // 确认对话框
    std::string msg = entry.isDirectory ?
        "Delete folder and all contents?\n\n" + entry.name :
        "Delete file?\n\n" + entry.name;
    
    ImGui::OpenPopup("Confirm Delete");
    if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", msg.c_str());
        ImGui::Separator();
        
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete", ImVec2(120, 0))) {
            if (!BeginOperation()) {
                ImGui::CloseCurrentPopup();
                return;
            }
            
            std::string path = currentPath_ == "\\" ? 
                "\\" + entry.name : currentPath_ + "\\" + entry.name;
            bool success = wfs_.DeleteEntry(currentPath_, entry.name, entry.isDirectory);
            
            EndOperation();
            
            if (success) {
                wfs_.Flush();
                selectedIndex_ = -1;
                RefreshList();
            } else {
                ShowErrorDialog("Delete Failed", "Could not delete the selected item.");
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
            ShowErrorDialog("Import Failed", "Could not import the file.");
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
            ShowErrorDialog("Export Failed", "Could not export the file.");
        }
    }
}

void WfsApp::ShowErrorDialog(const std::string& title, const std::string& message) {
    ImGui::OpenPopup(title.c_str());
    if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", message.c_str());
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
