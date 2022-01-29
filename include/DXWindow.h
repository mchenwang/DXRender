#ifndef __DXWINDOW_H__
#define __DXWINDOW_H__

#include "stdafx.h"
#include "SwapChain.h"
#include "CommandQueue.h"
#include "DescriptorHeap.h"
#include "Model.h"
#include "Camera.h"

using namespace DirectX;

class DXWindow
{
    const wchar_t* m_name;
    std::wstring m_assetsPath;

    std::shared_ptr<Model> m_model;
    std::shared_ptr<Camera> m_camera;

    // Use WARP adapter
    bool m_useWarp = false;

    uint32_t m_width;
    uint32_t m_height;

    // Set to true once the DX12 objects have been initialized.
    bool m_isInitialized = false;

    // Window handle.
    HWND m_hWnd = nullptr;
    // Window rectangle (used to toggle fullscreen state).
    RECT m_windowRect;

    // DirectX 12 Objects
    ComPtr<ID3D12Device2> m_device;

    std::shared_ptr<SwapChain> m_swapChain;
    std::shared_ptr<CommandQueue> m_commandQueue;
    std::shared_ptr<RTVDescriptorHeap> m_RTVHeap;
    std::shared_ptr<DSVDescriptorHeap> m_DSVHeap;

    ComPtr<ID3D12RootSignature> m_RootSignature;
    ComPtr<ID3D12PipelineState> m_PipelineState;
    ComPtr<ID3D12Resource> m_VertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
    ComPtr<ID3D12Resource> m_IndexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

    // Depth buffer.
    ComPtr<ID3D12Resource> m_DepthBuffer;

    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;

    // By default, use windowed mode.
    // Can be toggled with the Alt+Enter or F11
    bool m_fullscreen = false;

    // float m_FoV;

    DirectX::XMMATRIX m_ModelMatrix; // TODO
    // DirectX::XMMATRIX m_ViewMatrix;
    // DirectX::XMMATRIX m_ProjectionMatrix;

    std::wstring GetAssetFullPath(LPCWSTR assetName);

    void LoadPipeline();
    void UpdateBufferResource(
        ComPtr<ID3D12GraphicsCommandList> commandList,
        ID3D12Resource** pDestinationResource,
        ID3D12Resource** pIntermediateResource,
        size_t numElements, size_t elementSize, const void* bufferData,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    void LoadAssets();

    void UpdateWindowRect(uint32_t width, uint32_t height);
public:
    DXWindow(const wchar_t* name, uint32_t w = 1280, uint32_t h = 720) noexcept;
    ~DXWindow() = default;

    void ParseCommandLineArguments();
    void Init(HWND hWnd);
    void Update();
    void Render();
    void Resize(uint32_t width, uint32_t height);
    void ResizeDepthBuffer(uint32_t width, uint32_t height);
    void Destroy();

    LRESULT OnWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    void SetFullscreen(bool fullscreen);
    void SetVSync(bool VSync);

    HWND GetHandler() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    const wchar_t* GetName() const;

    bool IsInitialized() const;
    bool IsFullscreen() const;
    bool IsVSync() const;
};

#endif