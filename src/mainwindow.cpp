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
        10, 10, 250, 200, hWnd_, nullptr, hInstance_, nullptr);
    
    CreateWindowExA(0, "STATIC", "OTP File:", WS_CHILD | WS_VISIBLE,
        20, 20, 80, 20, hWnd_, nullptr, hInstance_, nullptr);
    hOtpEdit_ = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        20, 45, 160, 25, hWnd_, nullptr, hInstance_, nullptr);
    CreateWindowExA(0, "BUTTON", "...", WS_CHILD | WS_VISIBLE,
        185, 45, 30, 25, hWnd_, (HMENU)1001, hInstance_, nullptr);
    
    CreateWindowExA(0, "STATIC", "SEEPROM File:", WS_CHILD | WS_VISIBLE,
        20, 80, 100, 20, hWnd_, nullptr, hInstance_, nullptr);
    hSeepromEdit_ = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        20, 105, 160, 25, hWnd_, nullptr, hInstance_, nullptr);
    CreateWindowExA(0, "BUTTON", "...", WS_CHILD | WS_VISIBLE,
        185, 105, 30, 25, hWnd_, (HMENU)1002, hInstance_, nullptr);
    
    CreateWindowExA(0, "STATIC", "Device File:", WS_CHILD | WS_VISIBLE,
        20, 140, 100, 20, hWnd_, nullptr, hInstance_, nullptr);
    hDriveEdit_ = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        20, 165, 160, 25, hWnd_, nullptr, hInstance_, nullptr);
    CreateWindowExA(0, "BUTTON", "...", WS_CHILD | WS_VISIBLE,
        185, 165, 30, 25, hWnd_, (HMENU)1003, hInstance_, nullptr);
    
    hConnectBtn_ = CreateWindowExA(0, "BUTTON", "Connect", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 220, 80, 30, hWnd_, (HMENU)1010, hInstance_, nullptr);
    hDisconnectBtn_ = CreateWindowExA(0, "BUTTON", "Disconnect", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
        100, 220, 80, 30, hWnd_, (HMENU)1011, hInstance_, nullptr);
    
    // 当前路径标签
    hPathLabel_ = CreateWindowExA(0, "STATIC", "Current: \\", WS_CHILD | WS_VISIBLE | SS_SUNKEN,
        280, 10, 500, 25, hWnd_, nullptr, hInstance_, nullptr);
    
    // 文件列表 (ListView)
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

void MainWindow::ShowContextMenu(int x, int y) {
    if (!wfs_.IsConnected()) return;
    
    HMENU hMenu = CreatePopupMenu();
    
    // 如果有选中项
    if (!selectedName_.empty()) {
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
    
    // 总是显示导入选项
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
    ofn.lpstrDefExt = "";
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
        // 从路径提取文件名
        std::string filepath = filename;
        std::string name = filepath;
        size_t pos = filepath.find_last_of("\\/");
        if (pos != std::string::npos) {
            name = filepath.substr(pos + 1);
        }
        
        if (wfs_.ImportFile(filepath, currentPath_, name)) {
            wfs_.Flush();
            RefreshList();
            MessageBoxA(hWnd_, "File imported successfully!", "Success", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxA(hWnd_, "Failed to import file.", "Error", MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::OnConnect() {
    char otpPath[MAX_PATH], seepromPath[MAX_PATH], devicePath[MAX_PATH];
    GetWindowTextA(hOtpEdit_, otpPath, MAX_PATH);
    GetWindowTextA(hSeepromEdit_, seepromPath, MAX_PATH);
    GetWindowTextA(hDriveEdit_, devicePath, MAX_PATH);
    
    if (strlen(otpPath) == 0 || strlen(seepromPath) == 0 || strlen(devicePath) == 0) {
        MessageBoxA(hWnd_, "Please select all required files:\n- OTP File\n- SEEPROM File\n- Device File", 
                   "Missing Files", MB_OK | MB_ICONWARNING);
        return;
    }
    
    if (!FileExists(otpPath) || !FileExists(seepromPath) || !FileExists(devicePath)) {
        MessageBoxA(hWnd_, "One or more files not found.", "File Not Found", MB_OK | MB_ICONERROR);
        return;
    }
    
    if (!wfs_.Connect(otpPath, seepromPath, devicePath)) {
        MessageBoxA(hWnd_, "Failed to connect to WFS device.\n\nPossible reasons:\n- Invalid OTP/SEEPROM\n- Not a valid WFS device", 
                   "Connection Failed", MB_OK | MB_ICONERROR);
        return;
    }
    
    EnableWindow(hConnectBtn_, FALSE);
    EnableWindow(hDisconnectBtn_, TRUE);
    EnableWindow(hOtpEdit_, FALSE);
    EnableWindow(hSeepromEdit_, FALSE);
    EnableWindow(hDriveEdit_, FALSE);
    
    currentPath_ = "\\";
    RefreshList();
    MessageBoxA(hWnd_, "Connected successfully!", "Info", MB_OK | MB_ICONINFORMATION);
}

void MainWindow::OnDisconnect() {
    wfs_.Disconnect();
    
    EnableWindow(hConnectBtn_, TRUE);
    EnableWindow(hDisconnectBtn_, FALSE);
    EnableWindow(hOtpEdit_, TRUE);
    EnableWindow(hSeepromEdit_, TRUE);
    EnableWindow(hDriveEdit_, TRUE);
    
    ListView_DeleteAllItems(hFileList_);
    SetWindowTextA(hPathLabel_, "Current: \\");
    currentPath_ = "\\";
    selectedName_.clear();
    selectedPath_.clear();
    
    MessageBoxA(hWnd_, "Disconnected successfully.", "Info", MB_OK);
}

void MainWindow::OnDelete() {
    if (!wfs_.IsConnected() || selectedName_.empty()) return;
    
    std::string msg = selectedIsDir_ ? 
        ("Delete folder and all contents?\n\n" + selectedName_) :
        ("Delete file?\n\n" + selectedName_);
    
    if (MessageBoxA(hWnd_, msg.c_str(), "Confirm Delete", MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return;
    }
    
    if (wfs_.DeleteEntry(currentPath_, selectedName_, selectedIsDir_)) {
        wfs_.Flush();
        selectedName_.clear();
        selectedPath_.clear();
        RefreshList();
        MessageBoxA(hWnd_, "Deleted successfully!", "Success", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxA(hWnd_, "Failed to delete.", "Error", MB_OK | MB_ICONERROR);
    }
}

void MainWindow::NavigateTo(const std::string& name, bool isDir) {
    if (name == ".") {
        // 当前目录，什么都不做
        return;
    }
    
    if (name == "..") {
        // 返回上层
        if (currentPath_ == "\\") return;  // 已经是根目录
        
        size_t pos = currentPath_.find_last_of('\\');
        if (pos == 0) {
            currentPath_ = "\\";
        } else {
            currentPath_ = currentPath_.substr(0, pos);
        }
        RefreshList();
        return;
    }
    
    // 进入子目录
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
    
    // 更新路径标签
    std::string labelText = "Current: " + currentPath_;
    SetWindowTextA(hPathLabel_, labelText.c_str());
    
    LVITEMA lvi = {0};
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    
    int row = 0;
    
    // 添加 . 和 .. (除非是根目录)
    if (currentPath_ != "\\") {
        // .
        lvi.iItem = row;
        lvi.iSubItem = 0;
        lvi.pszText = (LPSTR)".";
        lvi.lParam = 0;
        ListView_InsertItem(hFileList_, &lvi);
        ListView_SetItemText(hFileList_, row, 1, (LPSTR)"<DIR>");
        ListView_SetItemText(hFileList_, row, 2, (LPSTR)"");
        row++;
        
        // ..
        lvi.iItem = row;
        lvi.pszText = (LPSTR)"..";
        ListView_InsertItem(hFileList_, &lvi);
        ListView_SetItemText(hFileList_, row, 1, (LPSTR)"<UP>");
        ListView_SetItemText(hFileList_, row, 2, (LPSTR)"");
        row++;
    }
    
    // 获取目录内容
    auto entries = wfs_.ListDirectory(currentPath_);
    
    // 先排序：文件夹在前，然后文件
    std::vector<DirEntry> dirs, files;
    for (const auto& e : entries) {
        if (e.is_directory) dirs.push_back(e);
        else files.push_back(e);
    }
    
    // 添加文件夹
    for (const auto& entry : dirs) {
        lvi.iItem = row;
        lvi.pszText = (LPSTR)entry.name.c_str();
        lvi.lParam = (LPARAM)new std::string(entry.name);
        ListView_InsertItem(hFileList_, &lvi);
        ListView_SetItemText(hFileList_, row, 1, (LPSTR)"<DIR>");
        ListView_SetItemText(hFileList_, row, 2, (LPSTR)"");
        row++;
    }
    
    // 添加文件
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
        // 可能是 . 或 ..
        char name[256] = {0};
        ListView_GetItemText(hFileList_, idx, 0, name, sizeof(name));
        selectedName_ = name;
    }
    
    // 判断类型
    char type[32] = {0};
    ListView_GetItemText(hFileList_, idx, 1, type, sizeof(type));
    selectedIsDir_ = (strcmp(type, "<DIR>") == 0 || strcmp(type, "<UP>") == 0 || selectedName_ == "." || selectedName_ == "..");
    
    // 计算完整路径
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
            
            if (id >= 1001 && id <= 1003) {
                OPENFILENAMEA ofn = {0};
                char filename[MAX_PATH] = {0};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hWnd_;
                ofn.lpstrFile = filename;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                
                if (GetOpenFileNameA(&ofn)) {
                    if (id == 1001) SetWindowTextA(hOtpEdit_, filename);
                    else if (id == 1002) SetWindowTextA(hSeepromEdit_, filename);
                    else if (id == 1003) SetWindowTextA(hDriveEdit_, filename);
                }
            }
            else if (id == 1010) OnConnect();
            else if (id == 1011) OnDisconnect();
            else if (id == 3001) { NavigateTo(selectedName_, true); }  // Open folder
            else if (id == 3002) OnExport();
            else if (id == 3003) OnDelete();
            else if (id == 3004) OnImport();
            break;
        }
        
        case WM_NOTIFY: {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if (pnmh->idFrom == 2000) {
                // 双击
                if (pnmh->code == NM_DBLCLK) {
                    UpdateSelection();
                    if (selectedName_ == "." || selectedName_ == "..") {
                        NavigateTo(selectedName_, true);
                    } else if (selectedIsDir_) {
                        NavigateTo(selectedName_, true);
                    }
                }
                // 右键菜单
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
                if (!selectedName_.empty() && selectedName_ != "." && selectedName_ != "..") {
                    OnDelete();
                }
            }
            else if (wParam == VK_RETURN) {
                UpdateSelection();
                if (selectedIsDir_) {
                    NavigateTo(selectedName_, true);
                }
            }
            else if (wParam == VK_BACK) {
                NavigateTo("..", true);
            }
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
            if (pos != std::string::npos) {
                name = filepath.substr(pos + 1);
            }
            
            std::string msg = "Import file to current directory?\n\n" + currentPath_ + "\\\n" + name;
            if (MessageBoxA(hWnd_, msg.c_str(), "Confirm Import", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                if (wfs_.ImportFile(filepath, currentPath_, name)) {
                    wfs_.Flush();
                    RefreshList();
                    MessageBoxA(hWnd_, "File imported!", "Success", MB_OK | MB_ICONINFORMATION);
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
    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
