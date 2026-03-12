#include "mainwindow.h"
#include <commdlg.h>
#include <shellapi.h>
#include <string>
#include <vector>

#pragma comment(lib, "comctl32.lib")

static MainWindow* g_pThis = nullptr;

MainWindow::MainWindow(HINSTANCE hInstance) : hInstance_(hInstance) {
    g_pThis = this;
}

MainWindow::~MainWindow() {
    g_pThis = nullptr;
}

int MainWindow::Run() {
    // 注册窗口类
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance_;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "WFSExplorerWindow";
    RegisterClassExA(&wc);
    
    // 初始化通用控件
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TREEVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
    
    // 创建主窗口
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
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}

void MainWindow::CreateControls() {
    // 左侧面板背景
    CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_GRAYRECT,
        10, 10, 250, 200, hWnd_, nullptr, hInstance_, nullptr);
    
    // OTP 选择
    CreateWindowExA(0, "STATIC", "OTP File:", WS_CHILD | WS_VISIBLE,
        20, 20, 80, 20, hWnd_, nullptr, hInstance_, nullptr);
    hOtpEdit_ = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        20, 45, 160, 25, hWnd_, nullptr, hInstance_, nullptr);
    CreateWindowExA(0, "BUTTON", "...", WS_CHILD | WS_VISIBLE,
        185, 45, 30, 25, hWnd_, (HMENU)1001, hInstance_, nullptr);
    
    // SEEPROM 选择
    CreateWindowExA(0, "STATIC", "SEEPROM File:", WS_CHILD | WS_VISIBLE,
        20, 80, 100, 20, hWnd_, nullptr, hInstance_, nullptr);
    hSeepromEdit_ = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        20, 105, 160, 25, hWnd_, nullptr, hInstance_, nullptr);
    CreateWindowExA(0, "BUTTON", "...", WS_CHILD | WS_VISIBLE,
        185, 105, 30, 25, hWnd_, (HMENU)1002, hInstance_, nullptr);
    
    // Device 选择
    CreateWindowExA(0, "STATIC", "Device File:", WS_CHILD | WS_VISIBLE,
        20, 140, 100, 20, hWnd_, nullptr, hInstance_, nullptr);
    hDriveEdit_ = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        20, 165, 160, 25, hWnd_, nullptr, hInstance_, nullptr);
    CreateWindowExA(0, "BUTTON", "...", WS_CHILD | WS_VISIBLE,
        185, 165, 30, 25, hWnd_, (HMENU)1003, hInstance_, nullptr);
    
    // 按钮
    hConnectBtn_ = CreateWindowExA(0, "BUTTON", "Connect", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 220, 80, 30, hWnd_, (HMENU)1010, hInstance_, nullptr);
    hDisconnectBtn_ = CreateWindowExA(0, "BUTTON", "Disconnect", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
        100, 220, 80, 30, hWnd_, (HMENU)1011, hInstance_, nullptr);
    
    // 删除按钮
    hDeleteBtn_ = CreateWindowExA(0, "BUTTON", "Delete Selected", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
        10, 260, 120, 30, hWnd_, (HMENU)1012, hInstance_, nullptr);
    
    // 文件树
    hFileTree_ = CreateWindowExA(WS_EX_CLIENTEDGE, WC_TREEVIEWA, "", 
        WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
        280, 10, 500, 540, hWnd_, (HMENU)2000, hInstance_, nullptr);
    
    // 启用拖放
    DragAcceptFiles(hWnd_, TRUE);
}

void MainWindow::OnConnect() {
    char otpPath[MAX_PATH], seepromPath[MAX_PATH], devicePath[MAX_PATH];
    GetWindowTextA(hOtpEdit_, otpPath, MAX_PATH);
    GetWindowTextA(hSeepromEdit_, seepromPath, MAX_PATH);
    GetWindowTextA(hDriveEdit_, devicePath, MAX_PATH);
    
    if (strlen(otpPath) == 0 || strlen(seepromPath) == 0 || strlen(devicePath) == 0) {
        MessageBoxA(hWnd_, "Please fill all fields", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    if (!wfs_.Connect(otpPath, seepromPath, devicePath)) {
        MessageBoxA(hWnd_, "Failed to connect to WFS device", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    EnableWindow(hConnectBtn_, FALSE);
    EnableWindow(hDisconnectBtn_, TRUE);
    EnableWindow(hOtpEdit_, FALSE);
    EnableWindow(hSeepromEdit_, FALSE);
    EnableWindow(hDriveEdit_, FALSE);
    
    RefreshTree();
    MessageBoxA(hWnd_, "Connected successfully!", "Info", MB_OK | MB_ICONINFORMATION);
}

void MainWindow::OnDisconnect() {
    wfs_.Disconnect();
    
    EnableWindow(hConnectBtn_, TRUE);
    EnableWindow(hDisconnectBtn_, FALSE);
    EnableWindow(hOtpEdit_, TRUE);
    EnableWindow(hSeepromEdit_, TRUE);
    EnableWindow(hDriveEdit_, TRUE);
    EnableWindow(hDeleteBtn_, FALSE);
    
    // 清空树
    TreeView_DeleteAllItems(hFileTree_);
    
    MessageBoxA(hWnd_, "Disconnected", "Info", MB_OK);
}

void MainWindow::OnDelete() {
    if (selectedPath_.empty()) return;
    
    std::string msg = selectedIsDir_ ? 
        ("Delete directory: " + selectedName_ + " and all contents?") :
        ("Delete file: " + selectedName_ + "?");
    
    if (MessageBoxA(hWnd_, msg.c_str(), "Confirm Delete", MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return;
    }
    
    if (wfs_.DeleteEntry(selectedParentPath_, selectedName_, selectedIsDir_)) {
        wfs_.Flush();
        RefreshTree();
        MessageBoxA(hWnd_, "Deleted successfully!", "Info", MB_OK);
    } else {
        MessageBoxA(hWnd_, "Failed to delete", "Error", MB_OK | MB_ICONERROR);
    }
}

void MainWindow::RefreshTree() {
    TreeView_DeleteAllItems(hFileTree_);
    
    TVINSERTSTRUCTA tvi = {0};
    tvi.hParent = TVI_ROOT;
    tvi.hInsertAfter = TVI_LAST;
    tvi.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
    tvi.item.pszText = (LPSTR)"\\";
    tvi.item.cChildren = 1;
    tvi.item.lParam = (LPARAM)new std::string("\\");
    
    HTREEITEM hRoot = TreeView_InsertItem(hFileTree_, &tvi);
    
    PopulateTreeItem(hRoot, "\\");
}

void MainWindow::PopulateTreeItem(HTREEITEM hParent, const std::string& path) {
    auto entries = wfs_.ListDirectory(path);
    
    for (const auto& entry : entries) {
        std::string fullPath = (path == "\\") ? ("\\" + entry.name) : (path + "\\" + entry.name);
        
        std::string displayName = entry.name;
        if (entry.is_directory) {
            displayName = "[" + entry.name + "]";
        }
        
        TVINSERTSTRUCTA tvi = {0};
        tvi.hParent = hParent;
        tvi.hInsertAfter = TVI_LAST;
        tvi.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
        tvi.item.pszText = (LPSTR)displayName.c_str();
        tvi.item.cChildren = entry.is_directory ? 1 : 0;
        tvi.item.lParam = (LPARAM)new std::string(fullPath);
        
        HTREEITEM hItem = TreeView_InsertItem(hFileTree_, &tvi);
        
        // 如果是目录，预填充子项
        if (entry.is_directory) {
            PopulateTreeItem(hItem, fullPath);
        }
    }
}

LRESULT MainWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            
            // 文件选择按钮
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
            // 连接按钮
            else if (id == 1010) {
                OnConnect();
            }
            // 断开按钮
            else if (id == 1011) {
                OnDisconnect();
            }
            // 删除按钮
            else if (id == 1012) {
                OnDelete();
            }
            break;
        }
        
        case WM_NOTIFY: {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if (pnmh->idFrom == 2000) {
                if (pnmh->code == TVN_SELCHANGED) {
                    LPNMTREEVIEWA pnmtv = (LPNMTREEVIEWA)lParam;
                    TVITEMA item = pnmtv->itemNew;
                    
                    if (item.lParam) {
                        std::string* pPath = (std::string*)item.lParam;
                        selectedPath_ = *pPath;
                        
                        // 计算父路径和名称
                        size_t pos = selectedPath_.find_last_of('\\');
                        if (pos == 0) {
                            selectedParentPath_ = "\\";
                            selectedName_ = selectedPath_.substr(1);
                        } else if (pos != std::string::npos) {
                            selectedParentPath_ = selectedPath_.substr(0, pos);
                            selectedName_ = selectedPath_.substr(pos + 1);
                        }
                        
                        selectedIsDir_ = (selectedName_[0] == '[');
                        if (selectedIsDir_) {
                            selectedName_ = selectedName_.substr(1, selectedName_.length() - 2);
                        }
                        
                        EnableWindow(hDeleteBtn_, TRUE);
                    }
                }
            }
            break;
        }
        
        case WM_KEYDOWN: {
            if (wParam == VK_DELETE && wfs_.IsConnected()) {
                OnDelete();
            }
            break;
        }
        
        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            
            if (!wfs_.IsConnected()) {
                MessageBoxA(hWnd_, "Please connect first", "Error", MB_OK | MB_ICONERROR);
                DragFinish(hDrop);
                break;
            }
            
            char droppedFile[MAX_PATH];
            DragQueryFileA(hDrop, 0, droppedFile, MAX_PATH);
            DragFinish(hDrop);
            
            // 导入文件到根目录
            std::string filename = droppedFile;
            size_t pos = filename.find_last_of("\\/");
            if (pos != std::string::npos) {
                filename = filename.substr(pos + 1);
            }
            
            if (wfs_.ImportFile(droppedFile, "\\", filename)) {
                wfs_.Flush();
                RefreshTree();
                MessageBoxA(hWnd_, "File imported!", "Info", MB_OK);
            } else {
                MessageBoxA(hWnd_, "Failed to import file", "Error", MB_OK | MB_ICONERROR);
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
