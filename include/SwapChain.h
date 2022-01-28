#ifndef __SWAPCHAIN_H__
#define __SWAPCHAIN_H__

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cstdint>
#include <memory>
#include "d3dx12.h"
#include "CommandQueue.h"
#include "DescriptorHeap.h"

using Microsoft::WRL::ComPtr;

class SwapChain
{
public:
    static constexpr uint8_t NUM_OF_FRAMES = 3;

    static FLOAT s_clearColor[4];
private:
    ComPtr<ID3D12Device2> m_device;

    UINT m_width;
    UINT m_height;

    bool m_VSync;
    bool m_tearingSupported;

    ComPtr<IDXGISwapChain4> m_swapChain;
    ComPtr<ID3D12Resource> m_backBuffers[NUM_OF_FRAMES];
    UINT m_currentBackBufferIndex;
    // uint64_t m_frameFenceValues[NUM_OF_FRAMES] = {};

    std::shared_ptr<CommandQueue> m_commandQueue;

public:
    SwapChain() = delete;
    SwapChain(ComPtr<ID3D12Device2> device, UINT width, UINT height, HWND hWnd, std::shared_ptr<CommandQueue>& commandQueue);

    void ClearRenderTarget(ComPtr<ID3D12GraphicsCommandList2> d3dCommandList, D3D12_CPU_DESCRIPTOR_HANDLE& rtv, D3D12_CPU_DESCRIPTOR_HANDLE& dsv);
    void Present(ComPtr<ID3D12GraphicsCommandList2> d3dCommandList);
    void Resize(UINT width, UINT height, std::shared_ptr<RTVDescriptorHeap>& rtvHeap);

    UINT GetCurrentBackBufferIndex() const;
    void UpdateRenderTargetViews(std::shared_ptr<RTVDescriptorHeap>& rtvHeap);

    bool IsTearingSupported() const;
    bool IsVSync() const;
    void SetVSync(bool VSync);
};

#endif