#include "DXWindow.h"
#include "helper.h"
#include <chrono>
#include <dxgidebug.h>
#include <algorithm>
#include "Application.h"

DXWindow::DXWindow(const wchar_t* name, uint32_t w, uint32_t h) noexcept
    : m_Name(name)
    , m_Width(w)
    , m_Height(h)
    , m_Viewport(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h))
    , m_ScissorRect(0, 0, static_cast<LONG>(w), static_cast<LONG>(h))
    , m_FoV(45.)
{
    m_AssetsPath = shader_path;
}

void DXWindow::ParseCommandLineArguments()
{
    int argc;
    wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
 
    for (size_t i = 0; i < argc; ++i)
    {
        if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0)
        {
            m_Width = ::wcstol(argv[++i], nullptr, 10);
        }
        if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0)
        {
            m_Height = ::wcstol(argv[++i], nullptr, 10);
        }
        if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
        {
            m_UseWarp = true;
        }
    }
 
    // Free memory allocated by CommandLineToArgvW
    ::LocalFree(argv);
}

std::wstring DXWindow::GetAssetFullPath(LPCWSTR assetName)
{
    return m_AssetsPath + assetName;
}

void DXWindow::LoadPipeline()
{
    m_device = Application::GetInstance()->GetDevice();
    m_commandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_swapChain = std::make_shared<SwapChain>(m_device, m_Width, m_Height, m_hWnd, m_commandQueue);
    m_RTVHeap = std::make_shared<RTVDescriptorHeap>(m_device, SwapChain::NUM_OF_FRAMES);
    m_DSVHeap = std::make_shared<DSVDescriptorHeap>(m_device, 1);
    
    m_swapChain->UpdateRenderTargetViews(m_RTVHeap);
}

void DXWindow::UpdateBufferResource(
    ComPtr<ID3D12GraphicsCommandList> commandList,
    ID3D12Resource** pDestinationResource,
    ID3D12Resource** pIntermediateResource,
    size_t numElements, size_t elementSize, const void* bufferData,
    D3D12_RESOURCE_FLAGS flags)
{
    size_t bufferSize = numElements * elementSize;

    // Create a committed resource for the GPU resource in a default heap.
    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(pDestinationResource)));

    // Create an committed resource for the upload.
    if (bufferData)
    {
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(pIntermediateResource)));

        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = bufferData;
        subresourceData.RowPitch = bufferSize;
        subresourceData.SlicePitch = subresourceData.RowPitch;

        UpdateSubresources(commandList.Get(),
            *pDestinationResource, *pIntermediateResource,
            0, 0, 1, &subresourceData);
    }
}
struct MVPData
{
    XMMATRIX mvp;
    XMMATRIX modelMatrixNegaTrans;
    XMMATRIX modelMatrix;
};
MVPData g_MVPCB;
struct PassData
{
    XMFLOAT4 lightPos = XMFLOAT4(-2.f, 2.f, 2.f, 1.f);
    XMFLOAT4 eyePos = XMFLOAT4(0.f, 0.f, 5.f, 1.f);
    XMFLOAT3 ambientColor = XMFLOAT3(0.5f, 0.5f, 0.5f);
    float lightIntensity = 5.f;
    XMFLOAT3 lightColor = XMFLOAT3(1.f, 1.f, 1.f);
    float spotPower = 128.f;
};
PassData g_passData;

void DXWindow::LoadAssets()
{
    // 1.
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        // A single 32-bit constant root parameter that is used by the vertex shader.
        CD3DX12_ROOT_PARAMETER1 rootParameters[2];
        rootParameters[0].InitAsConstants(sizeof(MVPData) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[1].InitAsConstants(sizeof(PassData) / 4, 1, 0);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);
        // rootSignatureDesc.Init(0, nullptr, 0, nullptr, rootSignatureFlags);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(
            0,
            signature->GetBufferPointer(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(&m_RootSignature)));
    }

    // 2.
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        ThrowIfFailed(D3DCompileFromFile(
            GetAssetFullPath(L"shaders.hlsl").c_str(),
            nullptr, nullptr, "VSMain", "vs_5_1",
            compileFlags, 0, &vertexShader, nullptr
            )
        );
        ThrowIfFailed(D3DCompileFromFile(
            GetAssetFullPath(L"shaders.hlsl").c_str(),
            nullptr, nullptr, "PSMain", "ps_5_1",
            compileFlags, 0, &pixelShader, nullptr
            )
        );

        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            // { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
            CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
            CD3DX12_PIPELINE_STATE_STREAM_VS VS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        } pipelineStateStream;

        D3D12_RT_FORMAT_ARRAY rtvFormats = {};
        rtvFormats.NumRenderTargets = 1;
        rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

        pipelineStateStream.pRootSignature = m_RootSignature.Get();
        pipelineStateStream.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        pipelineStateStream.RTVFormats = rtvFormats;

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {};
        pipelineStateStreamDesc.SizeInBytes = sizeof(PipelineStateStream);
        pipelineStateStreamDesc.pPipelineStateSubobjectStream = &pipelineStateStream;
        ThrowIfFailed(m_device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));
    }

    auto commandList = m_commandQueue->GetCommandList(m_PipelineState.Get());
    
    ComPtr<ID3D12Resource> intermediateVertexBuffer;
    ComPtr<ID3D12Resource> intermediateIndexBuffer;

    // 4.
    {
        m_model = Application::GetInstance()->GetModel();
        auto vertices = m_model->GetVertices();
        auto numVertices = m_model->GetVerticesNum();
        auto indicies = m_model->GetIndicies();
        auto numIndicies = m_model->GetIndiciesNum();

        // Upload vertex buffer data.
        UpdateBufferResource(commandList, &m_VertexBuffer, &intermediateVertexBuffer,
            numVertices, sizeof(Vertex), vertices.data());

        // Create the vertex buffer view.
        m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
        m_VertexBufferView.SizeInBytes = numVertices * sizeof(Vertex);
        m_VertexBufferView.StrideInBytes = sizeof(Vertex);

        commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);

        // Upload index buffer data.
        UpdateBufferResource(commandList, &m_IndexBuffer, &intermediateIndexBuffer,
            numIndicies, sizeof(uint32_t), indicies.data());

        // Create index buffer view.
        m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
        m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
        m_IndexBufferView.SizeInBytes = numIndicies * sizeof(uint32_t);

        commandList->IASetIndexBuffer(&m_IndexBufferView);
    }

    m_commandQueue->ExecuteCommandList(commandList);

    ResizeDepthBuffer(m_Width, m_Height);
}

void DXWindow::Init(HWND hWnd)
{
    m_hWnd = hWnd;
    
    // Initialize the global window rect variable.
    ::GetWindowRect(m_hWnd, &m_WindowRect);

    LoadPipeline();
    LoadAssets();
    
    m_IsInitialized = true;
}

void DXWindow::Destroy()
{
    m_commandQueue->Destory();
    m_device->Release();

    m_swapChain.reset();
    m_commandQueue.reset();
    m_RTVHeap.reset();
    m_DSVHeap.reset();

    m_RootSignature->Release();
    m_PipelineState->Release();
    m_VertexBuffer->Release();
    m_IndexBuffer->Release();
    m_DepthBuffer->Release();
}

HWND DXWindow::GetHandler() const
{
    return m_hWnd;
}
uint32_t DXWindow::GetWidth() const
{
    return m_Width;
}
uint32_t DXWindow::GetHeight() const
{
    return m_Height;
}
const wchar_t* DXWindow::GetName() const
{
    return m_Name;
}
bool DXWindow::IsInitialized() const
{
    return m_IsInitialized;
}
void DXWindow::SetVSync(bool VSync)
{
    m_swapChain->SetVSync(VSync);
}
bool DXWindow::IsVSync() const
{
    return m_swapChain->IsVSync();
}
bool DXWindow::IsFullscreen() const
{
    return m_Fullscreen;
}

void DXWindow::Update()
{
    static uint64_t frameCounter = 0;
    static double elapsedSeconds = 0.0;
    static std::chrono::high_resolution_clock clock;
    static auto t0 = clock.now();
    static double totalTime = 0.;

    frameCounter++;
    auto t1 = clock.now();
    auto deltaTime = t1 - t0;
    totalTime += deltaTime.count() * 1e-9;
    t0 = t1;
    elapsedSeconds += deltaTime.count() * 1e-9;
    if (elapsedSeconds > 1.0)
    {
        char buffer[500];
        auto fps = frameCounter / elapsedSeconds;
        sprintf_s(buffer, 500, "FPS: %f\n", fps);
        // OutputDebugStringA(buffer);
        // cout << buffer << "\n";
        SetWindowTextA(m_hWnd, buffer);
        frameCounter = 0;
        elapsedSeconds = 0.0;
    }

    // Update the model matrix.
    float angle = static_cast<float>(totalTime * 90.0);
    // float angle = 0.f;
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 0, 0);
    // auto scale = XMMatrixScaling(30.f, 30.f, 30.f);
    auto scale = XMMatrixScaling(1.f, 1.f, 1.f);
    auto rotation = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));
    // auto translation = XMMatrixTranslation(0.f, -3.f, 0.f);
    auto translation = XMMatrixTranslation(0.f, 0.f, 0.f);
    
    // DXMath 里，变换是行向量左乘矩阵
    // m_ModelMatrix = XMMatrixMultiply(XMMatrixMultiply(scale, rotation), translation); // C-style
    m_ModelMatrix = scale * rotation * translation;

    // Update the view matrix.
    const XMVECTOR eyePosition = XMLoadFloat4(&g_passData.eyePos);
    const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
    const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
    m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    // Update the projection matrix.
    float aspectRatio = m_Width / static_cast<float>(m_Height);
    m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FoV), aspectRatio, 0.01f, 100.0f);
}

void DXWindow::Render()
{
    auto commandList = m_commandQueue->GetCommandList(m_PipelineState.Get());
    
    // Set necessary state.
    commandList->SetGraphicsRootSignature(m_RootSignature.Get());
    commandList->RSSetViewports(1, &m_Viewport);
    commandList->RSSetScissorRects(1, &m_ScissorRect);

    auto RTVHandle = m_RTVHeap->GetDescriptorHandle(m_swapChain->GetCurrentBackBufferIndex());
    auto DSVHandle = m_DSVHeap->GetDescriptorHandle();
    m_swapChain->ClearRenderTarget(commandList, RTVHandle, DSVHandle);
    
    // Set obj
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
    commandList->IASetIndexBuffer(&m_IndexBufferView);

    // Update the MVP matrix
    // XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
    // mvpMatrix = XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix); // C-style
    // DXMath中矩阵是行主序，hlsl中是列主序，在C++层面做一层转置效率更高
    auto mvp = m_ModelMatrix * m_ViewMatrix * m_ProjectionMatrix;
    g_MVPCB.mvp = XMMatrixTranspose(mvp);
    // mvp.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);
    g_MVPCB.modelMatrixNegaTrans = XMMatrixInverse(nullptr, m_ModelMatrix);
    g_MVPCB.modelMatrix = XMMatrixTranspose(m_ModelMatrix);

    commandList->SetGraphicsRoot32BitConstants(0, sizeof(MVPData) / 4, &g_MVPCB, 0);
    commandList->SetGraphicsRoot32BitConstants(1, sizeof(PassData) / 4, &g_passData, 0);

    commandList->DrawIndexedInstanced(m_model->GetIndiciesNum(), 1, 0, 0, 0);

    m_swapChain->Present(commandList);
}

void DXWindow::SetFullscreen(bool fullscreen)
{
    if (m_Fullscreen != fullscreen)
    {
        m_Fullscreen = fullscreen;

        if (m_Fullscreen) // Switching to fullscreen.
        {
            // Store the current window dimensions so they can be restored 
            // when switching out of fullscreen state.
            ::GetWindowRect(m_hWnd, &m_WindowRect);
            // Set the window style to a borderless window so the client area fills
            // the entire screen.
            UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

            ::SetWindowLongW(m_hWnd, GWL_STYLE, windowStyle);
            // Query the name of the nearest display device for the window.
            // This is required to set the fullscreen dimensions of the window
            // when using a multi-monitor setup.
            HMONITOR hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitorInfo = {};
            monitorInfo.cbSize = sizeof(MONITORINFOEX);
            ::GetMonitorInfo(hMonitor, &monitorInfo);
            ::SetWindowPos(m_hWnd, HWND_TOP,
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(m_hWnd, SW_MAXIMIZE);

            m_Width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
            m_Height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
        }
        else
        {
            // Restore all the window decorators.
            ::SetWindowLongW(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

            ::SetWindowPos(m_hWnd, HWND_NOTOPMOST,
                m_WindowRect.left,
                m_WindowRect.top,
                m_WindowRect.right - m_WindowRect.left,
                m_WindowRect.bottom - m_WindowRect.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(m_hWnd, SW_NORMAL);

            m_Width = m_WindowRect.right - m_WindowRect.left;
            m_Height = m_WindowRect.bottom - m_WindowRect.top;
        }
        
        m_swapChain->Resize(m_Width, m_Height, m_RTVHeap);
        m_Viewport = CD3DX12_VIEWPORT(0.f, 0.f,
            static_cast<float>(m_Width), static_cast<float>(m_Height));
        m_ScissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(m_Width), static_cast<LONG>(m_Height));
        
        ResizeDepthBuffer(m_Width, m_Height);
    }
}

void DXWindow::Resize(uint32_t width, uint32_t height)
{
    if (m_Width != width || m_Height != height)
    {
        m_swapChain->Resize(width, height, m_RTVHeap);
        
        m_Width = std::max(1u, width);
        m_Height = std::max(1u, height);

        m_Viewport = CD3DX12_VIEWPORT(0.f, 0.f,
            static_cast<float>(m_Width), static_cast<float>(m_Height));
        m_ScissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(m_Width), static_cast<LONG>(m_Height));

        ResizeDepthBuffer(m_Width, m_Height);
    }
}

void DXWindow::ResizeDepthBuffer(uint32_t width, uint32_t height)
{
    // Flush any GPU commands that might be referencing the depth buffer.
    m_commandQueue->Flush();

    width = std::max(1u, width);
    height = std::max(1u, height);

    // Resize screen dependent resources.
    // Create a depth buffer.
    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil = { 1.0f, 0 };

    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
            1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optimizedClearValue,
        IID_PPV_ARGS(&m_DepthBuffer)
    ));

    // Update the depth-stencil view.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    dsv.Flags = D3D12_DSV_FLAG_NONE;

    m_device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsv,
        m_DSVHeap->GetDescriptorHandle());
}

LRESULT DXWindow::OnWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (IsInitialized())
    {
        switch (message)
        {
        case WM_PAINT:
            Update();
            Render();
            break;
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

            switch (wParam)
            {
            case 'V':
                SetVSync(!IsVSync());
                break;
            case VK_ESCAPE:
                ::PostQuitMessage(0);
                break;
            case VK_RETURN:
                if ( alt )
                {
            case VK_F11:
                SetFullscreen(!IsFullscreen());
                }
                break;
            }
        }
        break;
        // The default window procedure will play a system notification sound 
        // when pressing the Alt+Enter keyboard combination if this message is 
        // not handled.
        case WM_SYSCHAR:
        break;
        case WM_SIZE:
        {
            RECT clientRect = {};
            ::GetClientRect(hWnd, &clientRect);

            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;

            Resize(width, height);
        }
        break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            break;
        default:
            return ::DefWindowProcW(hWnd, message, wParam, lParam);
        }
    }
    else
    {
        return ::DefWindowProcW(hWnd, message, wParam, lParam);
    }

    return 0;
}