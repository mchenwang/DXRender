#include "SwapChain.h"
#include "helper.h"

FLOAT SwapChain::s_clearColor[4] = {0.f, 0.f, 0.f, 1.f};

static bool CheckTearingSupport()
{
    BOOL allowTearing = FALSE;

    // Rather than create the DXGI 1.5 factory interface directly, we create the
    // DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
    // graphics debugging tools which will not support the 1.5 factory interface 
    // until a future update.
    ComPtr<IDXGIFactory4> factory4;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
    {
        ComPtr<IDXGIFactory5> factory5;
        if (SUCCEEDED(factory4.As(&factory5)))
        {
            if (FAILED(factory5->CheckFeatureSupport(
                DXGI_FEATURE_PRESENT_ALLOW_TEARING, 
                &allowTearing, sizeof(allowTearing))))
            {
                allowTearing = FALSE;
            }
        }
    }

    return allowTearing == TRUE;
}

void SwapChain::ClearRenderTarget(ComPtr<ID3D12GraphicsCommandList2> d3dCommandList, D3D12_CPU_DESCRIPTOR_HANDLE& rtv, D3D12_CPU_DESCRIPTOR_HANDLE& dsv)
{
    auto backBuffer = m_backBuffers[m_currentBackBufferIndex];
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer.Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    d3dCommandList->ResourceBarrier(1, &barrier);

    d3dCommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    d3dCommandList->ClearRenderTargetView(rtv, s_clearColor, 0, nullptr);
    d3dCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
}

void SwapChain::Present(ComPtr<ID3D12GraphicsCommandList2> d3dCommandList)
{
    auto backBuffer = m_backBuffers[m_currentBackBufferIndex];
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    d3dCommandList->ResourceBarrier(1, &barrier);
    m_commandQueue->ExecuteCommandList(d3dCommandList);

    UINT syncInterval = m_VSync ? 1 : 0;
    UINT presentFlags = m_tearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    ThrowIfFailed(m_swapChain->Present(syncInterval, presentFlags));

    // [!!!保留]back buffer的资源应该和commandlist对应的commandallocator一致，commandallocator已经设置了一次fence
    // 这里应该就不需要再次signal和wait fence value了
    // m_frameFenceValues[m_currentBackBufferIndex] = m_commandQueue->Signal();
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    // m_commandQueue->WaitForFenceValue(m_frameFenceValues[m_currentBackBufferIndex]);
}

void SwapChain::Resize(UINT width, UINT height, std::shared_ptr<RTVDescriptorHeap>& rtvHeap)
{
    if (m_width == width && m_height == height) return ;
    
    m_width = std::max(1u, width);
    m_height = std::max(1u, height);
    
    m_commandQueue->Flush();
    for (int i = 0; i < NUM_OF_FRAMES; i++)
    {
        m_backBuffers[i].Reset();
        // m_frameFenceValues[i] = m_frameFenceValues[m_currentBackBufferIndex];
    }

    DXGI_SWAP_CHAIN_DESC desc = {};
    ThrowIfFailed(m_swapChain->GetDesc(&desc));
    ThrowIfFailed(m_swapChain->ResizeBuffers(NUM_OF_FRAMES,
        m_width, m_height, desc.BufferDesc.Format, desc.Flags));
    
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    UpdateRenderTargetViews(rtvHeap);
}

UINT SwapChain::GetCurrentBackBufferIndex() const
{
    return m_currentBackBufferIndex;
}

void SwapChain::UpdateRenderTargetViews(std::shared_ptr<RTVDescriptorHeap>& rtvHeap)
{
    auto rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetHeap()->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < NUM_OF_FRAMES; ++i)
    {
        // ComPtr<ID3D12Resource> backBuffer;
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i])));
        m_device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtvHandle);
        //  = backBuffer;
        rtvHandle.Offset(1, rtvDescriptorSize);
    }
}

bool SwapChain::IsTearingSupported() const
{
    return m_tearingSupported;
}
bool SwapChain::IsVSync() const
{
    return m_VSync;
}
void SwapChain::SetVSync(bool VSync)
{
    m_VSync = VSync;
}

SwapChain::SwapChain(ComPtr<ID3D12Device2> device, UINT width, UINT height, HWND hWnd,
    std::shared_ptr<CommandQueue>& commandQueue)
    : m_device(device)
    , m_width(width)
    , m_height(height)
    , m_VSync(false)
    , m_tearingSupported(CheckTearingSupport())
    , m_commandQueue(commandQueue)
{
    assert(hWnd);

    ComPtr<IDXGIFactory4> dxgiFactory4;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = NUM_OF_FRAMES;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    // It is recommended to always allow tearing if tearing support is available.
    swapChainDesc.Flags = m_tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
        commandQueue->GetCommandQueue().Get(),
        hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1));

    // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
    // will be handled manually.
    ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
    ThrowIfFailed(swapChain1.As(&m_swapChain));

    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
}
