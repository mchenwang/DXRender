#ifndef __COMMANDQUEUE_H__
#define __COMMANDQUEUE_H__

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <queue>

using Microsoft::WRL::ComPtr;

struct CommandAllocatorEntry
{
    uint64_t fenceValue;
    ComPtr<ID3D12CommandAllocator> commandAllocator;
};

class CommandQueue
{
private:
    

    D3D12_COMMAND_LIST_TYPE m_type;
    ComPtr<ID3D12Device2> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12Fence> m_fence;
    HANDLE m_fenceEvent;
    uint64_t m_fenceValue;
    std::queue<CommandAllocatorEntry> m_commandAllocatorQueue;
    std::queue<ComPtr<ID3D12GraphicsCommandList2>> m_commandListQueue;

public:
    CommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
    ~CommandQueue() = default;

    ComPtr<ID3D12GraphicsCommandList2> GetCommandList(ID3D12PipelineState* pPipelineState);

    uint64_t ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2> commandList);

    uint64_t Signal();
    bool IsFenceComplete(uint64_t fenceValue);
    void WaitForFenceValue(uint64_t fenceValue);
    void Flush();
    void Destory();

    ComPtr<ID3D12CommandQueue> GetCommandQueue() const;

private:
    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
    ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(ID3D12CommandAllocator* pCommandAllocator, ID3D12PipelineState* pPipelineState);
};

#endif