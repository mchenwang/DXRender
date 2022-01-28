#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "DXWindow.h"

class Application
{
    friend LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
public:
    static const wchar_t* S_WINDOW_CLASS_NAME;

private:
    std::wstring m_assetsPath;
    ComPtr<IDXGIAdapter4> m_adapter = nullptr;
    ComPtr<ID3D12Device2> m_device = nullptr;
    bool m_useWARP = false;

    std::shared_ptr<DXWindow> m_window = nullptr;
    bool m_windowCreated = false;

    std::shared_ptr<Model> m_model;

public:
    static void CreateInstance(HINSTANCE hInst);
    static Application* GetInstance();

    HWND CreateWindow(HINSTANCE hInst, DXWindow* pWindow);
    bool IsWindowCreated() const;

    int Run(std::shared_ptr<DXWindow> window);

    void SetWARP(bool isUse);
    void SetModel(std::shared_ptr<Model>& m_model);
    std::shared_ptr<Model> GetModel() const;

    ComPtr<ID3D12Device2> GetDevice();

private:
    Application(HINSTANCE hInst) noexcept;
    Application(Application&) = delete;
    Application& operator=(Application&) = delete;
    LRESULT OnWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    void CreateAdapter();
    void CreateDevice();
};

#endif