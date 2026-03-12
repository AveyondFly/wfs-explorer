#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <windows.h>
#include <commctrl.h>
#include "wfs_wrapper.h"

class MainWindow {
public:
    MainWindow(HINSTANCE hInstance);
    ~MainWindow();
    
    int Run();
    
private:
    HINSTANCE hInstance_;
    HWND hWnd_;
    HWND hOtpEdit_;
    HWND hSeepromEdit_;
    HWND hDriveEdit_;
    HWND hConnectBtn_;
    HWND hDisconnectBtn_;
    HWND hFileTree_;
    
    WfsManager wfs_;
    std::string selectedPath_;
    std::string selectedName_;
    std::string selectedParentPath_;
    bool selectedIsDir_ = false;
    
    void CreateControls();
    void ShowContextMenu(int x, int y);
    void OnConnect();
    void OnDisconnect();
    void OnDelete();
    void OnExport();
    void RefreshTree();
    void PopulateTreeItem(HTREEITEM hParent, const std::string& path);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif // MAINWINDOW_H
