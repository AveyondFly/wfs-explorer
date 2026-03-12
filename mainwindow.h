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
    HWND hDriveCombo_;
    HWND hConnectBtn_;
    HWND hDisconnectBtn_;
    HWND hFormatBtn_;
    HWND hPathLabel_;
    HWND hFileList_;
    
    WfsManager wfs_;
    std::string currentPath_;
    std::string selectedName_;
    std::string selectedPath_;
    bool selectedIsDir_ = false;
    
    void CreateControls();
    std::string GetSelectedDrive();
    void ShowContextMenu(int x, int y);
    void OnConnect();
    void OnDisconnect();
    void OnFormat();
    void OnDelete();
    void OnExport();
    void OnImport();
    void NavigateTo(const std::string& name, bool isDir);
    void RefreshList();
    void UpdateSelection();
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif // MAINWINDOW_H
