
#include "CommandQueue.h"
#include "helper.h"

CommandQueue::CommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    : m_device(device)
    , m_type(type)
    , m_fenceValue(0)
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    ThrowIfFailed(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue)));
    ThrowIfFailed(m_device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(m_fenceEvent && "Failed to create fence event handle.");
}

void CommandQueue::Destory()
{
    Flush();
    ::CloseHandle(m_fenceEvent);
}

uint64_t CommandQueue::Signal()
{
    uint64_t fenceValue = ++m_fenceValue;
    m_commandQueue->Signal(m_fence.Get(), fenceValue);
    return fenceValue;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
    return m_fence->GetCompletedValue() >= fenceValue;
}

void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
{
    if (!IsFenceComplete(fenceValue))
    {
        m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
        ::WaitForSingleObject(m_fenceEvent, DWORD_MAX);
    }
}

void CommandQueue::Flush()
{
    WaitForFenceValue(Signal());
}

ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator()
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(m_device->CreateCommandAllocator(m_type, IID_PPV_ARGS(&commandAllocator)));
    return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList2> CommandQueue::CreateCommandList(
    ID3D12CommandAllocator* pCommandAllocator,
    ID3D12PipelineState* pPipelineState)
{
    ComPtr<ID3D12GraphicsCommandList2> commandList;
    ThrowIfFailed(m_device->CreateCommandList(0,
        m_type,
        pCommandAllocator,
        pPipelineState, IID_PPV_ARGS(&commandList)));
    return commandList;
}

ComPtr<ID3D12GraphicsCommandList2> CommandQueue::GetCommandList(ID3D12PipelineState* pPipelineState)
{
    ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
    ComPtr<ID3D12GraphicsCommandList2> commandList = nullptr;

    if (!m_commandAllocatorQueue.empty() && IsFenceComplete(m_commandAllocatorQueue.front().fenceValue))
    {
        commandAllocator = m_commandAllocatorQueue.front().commandAllocator;
        m_commandAllocatorQueue.pop();
        ThrowIfFailed(commandAllocator->Reset());
    }
    else
    {
        commandAllocator = CreateCommandAllocator();
    }

    if (!m_commandListQueue.empty())
    {
        commandList = m_commandListQueue.front();
        m_commandListQueue.pop();
        ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pPipelineState));
    }
    else
    {
        commandList = CreateCommandList(commandAllocator.Get(), pPipelineState);
    }

    ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));
    return commandList;
}

uint64_t CommandQueue::ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
    commandList->Close();

    ID3D12CommandAllocator* commandAllocator;
    UINT dataSize = sizeof(commandAllocator);
    ThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

    ID3D12CommandList* const ppCommandLists[] = { commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    uint64_t fenceValue = Signal();
    m_commandAllocatorQueue.emplace(CommandAllocatorEntry{fenceValue, commandAllocator});
    m_commandListQueue.emplace(commandList);

    commandAllocator->Release();
    return fenceValue;
}

ComPtr<ID3D12CommandQueue> CommandQueue::GetCommandQueue() const
{
    return m_commandQueue;
}