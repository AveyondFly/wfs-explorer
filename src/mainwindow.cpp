#include "mainwindow.h"
#include <commdlg.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <sys/stat.h>

#pragma comment(lib, "comctl32.lib")

static MainWindow* g_pThis = nullptr;

static bool FileExists(const char* path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

MainWindow::MainWindow(HINSTANCE hInstance) : hInstance_(hInstance), currentPath_("\\") {
    g_pThis = this;
}

MainWindow::~MainWindow() {
    g_pThis = nullptr;
}

int MainWindow::Run() {
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance_;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "WFSExplorerWindow";
    RegisterClassExA(&wc);
    
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
    
    hWnd_ = CreateWindowExA(
        WS_EX_ACCEPTFILES,
        "WFSExplorerWindow",
        "WFS Explorer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        nullptr, nullptr, hInstance_, nullptr
    );
    
    SetWindowLongPtr(hWnd_, GWLP_USERDATA, (LONG_PTR)this);
    CreateControls();
    
    ShowWindow(hWnd_, SW_SHOW);
    UpdateWindow(hWnd_);
    
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}

void MainWindow::CreateControls() {
    // 左侧面板
    CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_GRAYRECT,
        10, 10, 250, 180, hWnd_, nullptr, hInstance_, nullptr);
    
    // OTP
    CreateWindowExA(0, "STATIC", "OTP File:", WS_CHILD | WS_VISIBLE,
        20, 20, 80, 20, hWnd_, nullptr, hInstance_, nullptr);
    hOtpEdit_ = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        20, 45, 160, 25, hWnd_, nullptr, hInstance_, nullptr);
    CreateWindowExA(0, "BUTTON", "...", WS_CHILD | WS_VISIBLE,
        185, 45, 30, 25, hWnd_, (HMENU)1001, hInstance_, nullptr);
    
    // SEEPROM
    CreateWindowExA(0, "STATIC", "SEEPROM File:", WS_CHILD | WS_VISIBLE,
        20, 80, 100, 20, hWnd_, nullptr, hInstance_, nullptr);
    hSeepromEdit_ = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        20, 105, 160, 25, hWnd_, nullptr, hInstance_, nullptr);
    CreateWindowExA(0, "BUTTON", "...", WS_CHILD | WS_VISIBLE,
        185, 105, 30, 25, hWnd_, (HMENU)1002, hInstance_, nullptr);
    
    // Drive 盘符选择
    CreateWindowExA(0, "STATIC", "Wii U Partition:", WS_CHILD | WS_VISIBLE,
        20, 140, 120, 20, hWnd_, nullptr, hInstance_, nullptr);
    hDriveCombo_ = CreateWindowExA(0, "COMBOBOX", "", 
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        20, 165, 195, 200, hWnd_, (HMENU)1003, hInstance_, nullptr);
    
    // 扫描可用盘符
    char drives[256];
    GetLogicalDriveStringsA(sizeof(drives), drives);
    char* p = drives;
    while (*p) {
        std::string drive = p;
        // 只显示固定磁盘
        if (GetDriveTypeA(drive.c_str()) == DRIVE_FIXED) {
            SendMessageA(hDriveCombo_, CB_ADDSTRING, 0, (LPARAM)drive.c_str());
        }
        p += drive.length() + 1;
    }
    // 默认选择第一个
    SendMessageA(hDriveCombo_, CB_SETCURSEL, 0, 0);
    
    // 按钮
    hConnectBtn_ = CreateWindowExA(0, "BUTTON", "Connect", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 200, 75, 30, hWnd_, (HMENU)1010, hInstance_, nullptr);
    hDisconnectBtn_ = CreateWindowExA(0, "BUTTON", "Disconnect", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
        90, 200, 75, 30, hWnd_, (HMENU)1011, hInstance_, nullptr);
    hFormatBtn_ = CreateWindowExA(0, "BUTTON", "Format", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        170, 200, 75, 30, hWnd_, (HMENU)1012, hInstance_, nullptr);
    
    // 当前路径标签
    hPathLabel_ = CreateWindowExA(0, "STATIC", "Select drive and click Connect or Format", WS_CHILD | WS_VISIBLE | SS_SUNKEN,
        280, 10, 500, 25, hWnd_, nullptr, hInstance_, nullptr);
    
    // 文件列表
    hFileList_ = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "", 
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
        280, 40, 500, 510, hWnd_, (HMENU)2000, hInstance_, nullptr);
    
    // 设置列表列
    LVCOLUMNA lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    
    lvc.pszText = (LPSTR)"Name";
    lvc.cx = 280;
    ListView_InsertColumn(hFileList_, 0, &lvc);
    
    lvc.pszText = (LPSTR)"Type";
    lvc.cx = 80;
    ListView_InsertColumn(hFileList_, 1, &lvc);
    
    lvc.pszText = (LPSTR)"Size";
    lvc.cx = 100;
    ListView_InsertColumn(hFileList_, 2, &lvc);
    
    DragAcceptFiles(hWnd_, TRUE);
}

std::string MainWindow::GetSelectedDrive() {
    char drive[16] = {0};
    int sel = SendMessageA(hDriveCombo_, CB_GETCURSEL, 0, 0);
    SendMessageA(hDriveCombo_, CB_GETLBTEXT, sel, (LPARAM)drive);
    return std::string(drive);
}

void MainWindow::ShowContextMenu(int x, int y) {
    if (!wfs_.IsConnected()) return;
    
    HMENU hMenu = CreatePopupMenu();
    
    if (!selectedName_.empty() && selectedName_ != "." && selectedName_ != "..") {
        if (selectedIsDir_) {
            AppendMenuA(hMenu, MF_STRING, 3001, "Open folder");
            AppendMenuA(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuA(hMenu, MF_STRING, 3002, "Export folder...");
        } else {
            AppendMenuA(hMenu, MF_STRING, 3002, "Export file...");
        }
        AppendMenuA(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuA(hMenu, MF_STRING, 3003, "Delete");
    }
    
    AppendMenuA(hMenu, MF_STRING, 3004, "Import file here...");
    
    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, 0, hWnd_, nullptr);
    DestroyMenu(hMenu);
}

void MainWindow::OnExport() {
    if (!wfs_.IsConnected() || selectedName_.empty()) return;
    
    OPENFILENAMEA ofn = {0};
    char filename[MAX_PATH] = {0};
    strncpy(filename, selectedName_.c_str(), MAX_PATH - 1);
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd_;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    
    if (GetSaveFileNameA(&ofn)) {
        if (wfs_.ExportFile(selectedPath_, filename)) {
            MessageBoxA(hWnd_, "Export successful!", "Success", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxA(hWnd_, "Failed to export.", "Error", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::OnImport() {
    if (!wfs_.IsConnected()) return;
    
    OPENFILENAMEA ofn = {0};
    char filename[MAX_PATH] = {0};
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd_;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    
    if (GetOpenFileNameA(&ofn)) {
        std::string filepath = filename;
        std::string name = filepath;
        size_t pos = filepath.find_last_of("\\/");
        if (pos != std::string::npos) name = filepath.substr(pos + 1);
        
        if (wfs_.ImportFile(filepath, currentPath_, name)) {
            wfs_.Flush();
            RefreshList();
            MessageBoxA(hWnd_, "File imported!", "Success", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxA(hWnd_, "Failed to import.", "Error", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::OnConnect() {
    char otpPath[MAX_PATH], seepromPath[MAX_PATH];
    GetWindowTextA(hOtpEdit_, otpPath, MAX_PATH);
    GetWindowTextA(hSeepromEdit_, seepromPath, MAX_PATH);
    
    if (strlen(otpPath) == 0 || strlen(seepromPath) == 0) {
        MessageBoxA(hWnd_, "Please select OTP and SEEPROM files.", 
                   "Missing Files", MB_OK | MB_ICONWARNING);
        return;
    }
    
    if (!FileExists(otpPath) || !FileExists(seepromPath)) {
        MessageBoxA(hWnd_, "OTP or SEEPROM file not found.", "File Not Found", MB_OK | MB_ICONERROR);
        return;
    }
    
    std::string drive = GetSelectedDrive();
    if (drive.empty()) {
        MessageBoxA(hWnd_, "Please select a drive.", "No Drive", MB_OK | MB_ICONWARNING);
        return;
    }
    
    // 构建设备路径 (\\.\X:)
    std::string devicePath = "\\\\.\\" + drive.substr(0, 2);
    
    if (!wfs_.Connect(otpPath, seepromPath, devicePath)) {
        MessageBoxA(hWnd_, "Failed to connect.\n\nPossible reasons:\n- Invalid OTP/SEEPROM\n- Not a Wii U partition\n- Drive is in use", 
                   "Connection Failed", MB_OK | MB_ICONERROR);
        return;
    }
    
    EnableWindow(hConnectBtn_, FALSE);
    EnableWindow(hDisconnectBtn_, TRUE);
    EnableWindow(hFormatBtn_, FALSE);
    EnableWindow(hOtpEdit_, FALSE);
    EnableWindow(hSeepromEdit_, FALSE);
    EnableWindow(hDriveCombo_, FALSE);
    
    currentPath_ = "\\";
    RefreshList();
    MessageBoxA(hWnd_, "Connected successfully!", "Info", MB_OK | MB_ICONINFORMATION);
}

void MainWindow::OnDisconnect() {
    wfs_.Disconnect();
    
    EnableWindow(hConnectBtn_, TRUE);
    EnableWindow(hDisconnectBtn_, FALSE);
    EnableWindow(hFormatBtn_, TRUE);
    EnableWindow(hOtpEdit_, TRUE);
    EnableWindow(hSeepromEdit_, TRUE);
    EnableWindow(hDriveCombo_, TRUE);
    
    ListView_DeleteAllItems(hFileList_);
    SetWindowTextA(hPathLabel_, "Select drive and click Connect or Format");
    currentPath_ = "\\";
    selectedName_.clear();
    selectedPath_.clear();
    
    MessageBoxA(hWnd_, "Disconnected.", "Info", MB_OK);
}

void MainWindow::OnFormat() {
    char otpPath[MAX_PATH], seepromPath[MAX_PATH];
    GetWindowTextA(hOtpEdit_, otpPath, MAX_PATH);
    GetWindowTextA(hSeepromEdit_, seepromPath, MAX_PATH);
    
    if (strlen(otpPath) == 0 || strlen(seepromPath) == 0) {
        MessageBoxA(hWnd_, "Please select OTP and SEEPROM files.", 
                   "Missing Files", MB_OK | MB_ICONWARNING);
        return;
    }
    
    if (!FileExists(otpPath) || !FileExists(seepromPath)) {
        MessageBoxA(hWnd_, "OTP or SEEPROM file not found.", "File Not Found", MB_OK | MB_ICONERROR);
        return;
    }
    
    std::string drive = GetSelectedDrive();
    if (drive.empty()) {
        MessageBoxA(hWnd_, "Please select a drive.", "No Drive", MB_OK | MB_ICONWARNING);
        return;
    }
    
    // 格式化警告对话框
    int result = MessageBoxA(hWnd_, 
        "!!! WARNING - HIGH RISK OPERATION !!!\n\n"
        "Formatting will ERASE ALL DATA on the selected partition!\n\n"
        "This action cannot be undone!\n\n"
        "Make sure you have selected the correct Wii U partition.\n\n"
        "Do you want to continue?",
        "!!! FORMAT WARNING !!!",
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    
    if (result != IDYES) return;
    
    // 二次确认
    result = MessageBoxA(hWnd_,
        "FINAL WARNING!\n\n"
        "ALL DATA WILL BE LOST!\n\n"
        "Are you absolutely sure you want to format this partition?",
        "!!! FINAL WARNING !!!",
        MB_YESNO | MB_ICONSTOP | MB_DEFBUTTON2);
    
    if (result != IDYES) return;
    
    // 构建设备路径 (\\.\X:)
    std::string devicePath = "\\\\.\\" + drive.substr(0, 2);
    
    // 执行格式化
    if (!wfs_.Format(otpPath, seepromPath, devicePath)) {
        MessageBoxA(hWnd_, "Failed to format.\n\nPossible reasons:\n- Invalid OTP/SEEPROM\n- Drive is in use or write-protected\n- Insufficient permissions (Run as Administrator)", 
                   "Format Failed", MB_OK | MB_ICONERROR);
        return;
    }
    
    // 格式化成功，自动连接
    EnableWindow(hConnectBtn_, FALSE);
    EnableWindow(hDisconnectBtn_, TRUE);
    EnableWindow(hFormatBtn_, FALSE);
    EnableWindow(hOtpEdit_, FALSE);
    EnableWindow(hSeepromEdit_, FALSE);
    EnableWindow(hDriveCombo_, FALSE);
    
    currentPath_ = "\\";
    RefreshList();
    MessageBoxA(hWnd_, "Format completed successfully!\n\nPartition is now connected.", "Success", MB_OK | MB_ICONINFORMATION);
}

void MainWindow::OnDelete() {
    if (!wfs_.IsConnected() || selectedName_.empty()) return;
    if (selectedName_ == "." || selectedName_ == "..") return;
    
    std::string msg = selectedIsDir_ ? 
        ("Delete folder and all contents?\n\n" + selectedName_) :
        ("Delete file?\n\n" + selectedName_);
    
    if (MessageBoxA(hWnd_, msg.c_str(), "Confirm Delete", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    
    if (wfs_.DeleteEntry(currentPath_, selectedName_, selectedIsDir_)) {
        wfs_.Flush();
        selectedName_.clear();
        selectedPath_.clear();
        RefreshList();
        MessageBoxA(hWnd_, "Deleted!", "Success", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxA(hWnd_, "Failed to delete.", "Error", MB_OK | MB_ICONERROR);
    }
}

void MainWindow::NavigateTo(const std::string& name, bool isDir) {
    if (name == ".") return;
    
    if (name == "..") {
        if (currentPath_ == "\\") return;
        
        size_t pos = currentPath_.find_last_of('\\');
        if (pos == 0) {
            currentPath_ = "\\";
        } else {
            currentPath_ = currentPath_.substr(0, pos);
        }
        RefreshList();
        return;
    }
    
    if (!isDir) return;
    
    if (currentPath_ == "\\") {
        currentPath_ = "\\" + name;
    } else {
        currentPath_ = currentPath_ + "\\" + name;
    }
    RefreshList();
}

void MainWindow::RefreshList() {
    ListView_DeleteAllItems(hFileList_);
    
    if (!wfs_.IsConnected()) return;
    
    std::string labelText = "Current: " + currentPath_;
    SetWindowTextA(hPathLabel_, labelText.c_str());
    
    LVITEMA lvi = {0};
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    int row = 0;
    
    // 添加 .. (除非是根目录)
    if (currentPath_ != "\\") {
        lvi.iItem = row;
        lvi.pszText = (LPSTR)"..";
        lvi.lParam = 0;
        ListView_InsertItem(hFileList_, &lvi);
        ListView_SetItemText(hFileList_, row, 1, (LPSTR)"<UP>");
        ListView_SetItemText(hFileList_, row, 2, (LPSTR)"");
        row++;
    }
    
    auto entries = wfs_.ListDirectory(currentPath_);
    
    std::vector<DirEntry> dirs, files;
    for (const auto& e : entries) {
        if (e.is_directory) dirs.push_back(e);
        else files.push_back(e);
    }
    
    for (const auto& entry : dirs) {
        lvi.iItem = row;
        lvi.pszText = (LPSTR)entry.name.c_str();
        lvi.lParam = (LPARAM)new std::string(entry.name);
        ListView_InsertItem(hFileList_, &lvi);
        ListView_SetItemText(hFileList_, row, 1, (LPSTR)"<DIR>");
        ListView_SetItemText(hFileList_, row, 2, (LPSTR)"");
        row++;
    }
    
    char sizeStr[32];
    for (const auto& entry : files) {
        lvi.iItem = row;
        lvi.pszText = (LPSTR)entry.name.c_str();
        lvi.lParam = (LPARAM)new std::string(entry.name);
        ListView_InsertItem(hFileList_, &lvi);
        ListView_SetItemText(hFileList_, row, 1, (LPSTR)"File");
        snprintf(sizeStr, sizeof(sizeStr), "%u", entry.size);
        ListView_SetItemText(hFileList_, row, 2, sizeStr);
        row++;
    }
}

void MainWindow::UpdateSelection() {
    int idx = ListView_GetNextItem(hFileList_, -1, LVNI_SELECTED);
    if (idx == -1) {
        selectedName_.clear();
        selectedPath_.clear();
        return;
    }
    
    LVITEMA lvi = {0};
    lvi.iItem = idx;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_PARAM;
    ListView_GetItem(hFileList_, &lvi);
    
    if (lvi.lParam) {
        selectedName_ = *(std::string*)lvi.lParam;
    } else {
        char name[256] = {0};
        ListView_GetItemText(hFileList_, idx, 0, name, sizeof(name));
        selectedName_ = name;
    }
    
    char type[32] = {0};
    ListView_GetItemText(hFileList_, idx, 1, type, sizeof(type));
    selectedIsDir_ = (strcmp(type, "<DIR>") == 0 || strcmp(type, "<UP>") == 0);
    
    if (currentPath_ == "\\") {
        selectedPath_ = "\\" + selectedName_;
    } else {
        selectedPath_ = currentPath_ + "\\" + selectedName_;
    }
}

LRESULT MainWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            
            if (id == 1001 || id == 1002) {
                OPENFILENAMEA ofn = {0};
                char filename[MAX_PATH] = {0};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hWnd_;
                ofn.lpstrFile = filename;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                
                if (GetOpenFileNameA(&ofn)) {
                    if (id == 1001) SetWindowTextA(hOtpEdit_, filename);
                    else SetWindowTextA(hSeepromEdit_, filename);
                }
            }
            else if (id == 1010) OnConnect();
            else if (id == 1011) OnDisconnect();
            else if (id == 1012) OnFormat();
            else if (id == 3001) { UpdateSelection(); NavigateTo(selectedName_, true); }
            else if (id == 3002) OnExport();
            else if (id == 3003) OnDelete();
            else if (id == 3004) OnImport();
            break;
        }
        
        case WM_NOTIFY: {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if (pnmh->idFrom == 2000) {
                if (pnmh->code == NM_DBLCLK) {
                    UpdateSelection();
                    if (selectedIsDir_) NavigateTo(selectedName_, true);
                }
                else if (pnmh->code == NM_RCLICK) {
                    UpdateSelection();
                    POINT pt;
                    GetCursorPos(&pt);
                    ShowContextMenu(pt.x, pt.y);
                }
            }
            break;
        }
        
        case WM_KEYDOWN: {
            if (wParam == VK_DELETE && wfs_.IsConnected()) {
                UpdateSelection();
                if (!selectedName_.empty() && selectedName_ != "." && selectedName_ != "..") OnDelete();
            }
            else if (wParam == VK_RETURN) {
                UpdateSelection();
                if (selectedIsDir_) NavigateTo(selectedName_, true);
            }
            else if (wParam == VK_BACK) NavigateTo("..", true);
            break;
        }
        
        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            
            if (!wfs_.IsConnected()) {
                MessageBoxA(hWnd_, "Please connect first.", "Not Connected", MB_OK | MB_ICONWARNING);
                DragFinish(hDrop);
                break;
            }
            
            char droppedFile[MAX_PATH];
            DragQueryFileA(hDrop, 0, droppedFile, MAX_PATH);
            DragFinish(hDrop);
            
            if (!FileExists(droppedFile)) {
                MessageBoxA(hWnd_, "File not found.", "Error", MB_OK | MB_ICONERROR);
                break;
            }
            
            std::string filepath = droppedFile;
            std::string name = filepath;
            size_t pos = filepath.find_last_of("\\/");
            if (pos != std::string::npos) name = filepath.substr(pos + 1);
            
            std::string msgText = "Import to:\n" + currentPath_ + "\\\n" + name + " ?";
            if (MessageBoxA(hWnd_, msgText.c_str(), "Import", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                if (wfs_.ImportFile(filepath, currentPath_, name)) {
                    wfs_.Flush();
                    RefreshList();
                    MessageBoxA(hWnd_, "Imported!", "Success", MB_OK | MB_ICONINFORMATION);
                } else {
                    MessageBoxA(hWnd_, "Failed to import.", "Error", MB_OK | MB_ICONERROR);
                }
            }
            break;
        }
        
        case WM_DESTROY: {
            DragAcceptFiles(hWnd_, FALSE);
            PostQuitMessage(0);
            break;
        }
        
        default:
            return DefWindowProc(hWnd_, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK MainWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = (MainWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (pThis) return pThis->HandleMessage(msg, wParam, lParam);
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
