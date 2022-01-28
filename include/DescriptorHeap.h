#ifndef __DESCRIPTORHEAP_H__
#define __DESCRIPTORHEAP_H__

#include <d3d12.h>
#include <wrl.h>
#include "d3dx12.h"
using Microsoft::WRL::ComPtr;

class DescriptorHeap
{
protected:
    ComPtr<ID3D12DescriptorHeap> m_heap;
    UINT m_numDescriptors;
    DescriptorHeap(ComPtr<ID3D12Device2> device, UINT num, D3D12_DESCRIPTOR_HEAP_TYPE type,
        D3D12_DESCRIPTOR_HEAP_FLAGS flag);
public:
    // virtual D3D12_DESCRIPTOR_HEAP_TYPE GetType() const = 0;
    // virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const = 0;
    ComPtr<ID3D12DescriptorHeap> GetHeap() const;
};

class RTVDescriptorHeap : public DescriptorHeap
{
public:
    static const D3D12_DESCRIPTOR_HEAP_TYPE S_TYPE = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
private:
    UINT m_RTVDescriptorSize;
public:
    RTVDescriptorHeap(ComPtr<ID3D12Device2> device, UINT num, D3D12_DESCRIPTOR_HEAP_FLAGS flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    D3D12_DESCRIPTOR_HEAP_TYPE GetType() const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(UINT currentBackBufferIndex) const;
};

class DSVDescriptorHeap : public DescriptorHeap
{
public:
    static const D3D12_DESCRIPTOR_HEAP_TYPE S_TYPE = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
public:
    DSVDescriptorHeap(ComPtr<ID3D12Device2> device, UINT num, D3D12_DESCRIPTOR_HEAP_FLAGS flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    D3D12_DESCRIPTOR_HEAP_TYPE GetType() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const;
};

#endif