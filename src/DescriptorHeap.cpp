#include "DescriptorHeap.h"
#include "helper.h"

DescriptorHeap::DescriptorHeap(ComPtr<ID3D12Device2> device, UINT num, D3D12_DESCRIPTOR_HEAP_TYPE type,
        D3D12_DESCRIPTOR_HEAP_FLAGS flag)
    : m_numDescriptors(num)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = num;
    desc.Type = type;
    desc.Flags = flag;
    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap)));
}

RTVDescriptorHeap::RTVDescriptorHeap(ComPtr<ID3D12Device2> device, UINT num,
        D3D12_DESCRIPTOR_HEAP_FLAGS flag)
    : DescriptorHeap(device, num, S_TYPE, flag)
{
    m_RTVDescriptorSize = device->GetDescriptorHandleIncrementSize(S_TYPE);
}

DSVDescriptorHeap::DSVDescriptorHeap(ComPtr<ID3D12Device2> device, UINT num,
        D3D12_DESCRIPTOR_HEAP_FLAGS flag)
    : DescriptorHeap(device, num, S_TYPE, flag)
{
}

ComPtr<ID3D12DescriptorHeap> DescriptorHeap::GetHeap() const
{
    return m_heap;
}

D3D12_DESCRIPTOR_HEAP_TYPE RTVDescriptorHeap::GetType() const
{
    return S_TYPE;
}
D3D12_DESCRIPTOR_HEAP_TYPE DSVDescriptorHeap::GetType() const
{
    return S_TYPE;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE RTVDescriptorHeap::GetDescriptorHandle(UINT currentBackBufferIndex) const
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_heap->GetCPUDescriptorHandleForHeapStart(), 
        currentBackBufferIndex, m_RTVDescriptorSize);
    return rtv;
}
D3D12_CPU_DESCRIPTOR_HANDLE DSVDescriptorHeap::GetDescriptorHandle() const
{
    return m_heap->GetCPUDescriptorHandleForHeapStart();
}