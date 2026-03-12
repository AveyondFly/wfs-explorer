#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <windows.h>
#include <commctrl.h>
#include <string>
#include "wfs_wrapper.h"

class MainWindow {
public:
    MainWindow(HINSTANCE hInstance);
    ~MainWindow();
    
    int Run();
    
private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    
    void CreateControls();
    void OnConnect();
    void OnDisconnect();
    void OnDelete();
    void RefreshTree();
    void PopulateTreeItem(HTREEITEM hParent, const std::string& path);
    
    // 控件
    HWND hWnd_;
    HWND hOtpEdit_, hSeepromEdit_, hDriveEdit_;
    HWND hConnectBtn_, hDisconnectBtn_, hDeleteBtn_;
    HWND hFileTree_;
    
    HINSTANCE hInstance_;
    WfsManager wfs_;
    
    // 当前选中的路径
    std::string selectedPath_;
    std::string selectedParentPath_;
    std::string selectedName_;
    bool selectedIsDir_;
};

#endif // MAINWINDOW_H
