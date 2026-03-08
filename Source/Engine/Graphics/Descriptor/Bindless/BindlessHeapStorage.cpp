#include "pch.h"
#include "BindlessHeapStorage.h"

namespace tme::sys::graphics {

void BindlessHeapStorage::Init(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
    if (!device) {
        LOG_ERROR("Graphics", "BindlessHeapStorage::Init: Device is null");
        return;
    }
    device_ = device;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = BindlessHeapLayout::kTotalCount;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.NodeMask = 0;

    HRESULT hr = device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap_));
    if (FAILED(hr)) {
        LOG_ERROR("Graphics", "BindlessHeapStorage: CreateDescriptorHeap failed");
        return;
    }

    descriptorSize_ = device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    LOG_INFO("Graphics", "BindlessHeapStorage created",
        "totalSlots", BindlessHeapLayout::kTotalCount);
}

D3D12_CPU_DESCRIPTOR_HANDLE BindlessHeapStorage::GetCpuHandle(uint32_t absoluteIndex) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE h = heap_->GetCPUDescriptorHandleForHeapStart();
    h.ptr += static_cast<SIZE_T>(absoluteIndex) * descriptorSize_;
    return h;
}

D3D12_GPU_DESCRIPTOR_HANDLE BindlessHeapStorage::GetGpuHandle(uint32_t absoluteIndex) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE h = heap_->GetGPUDescriptorHandleForHeapStart();
    h.ptr += static_cast<UINT64>(absoluteIndex) * descriptorSize_;
    return h;
}

D3D12_GPU_DESCRIPTOR_HANDLE BindlessHeapStorage::GetSRVGpuBase() const
{
    return GetGpuHandle(BindlessHeapLayout::kSrvBase);
}

D3D12_GPU_DESCRIPTOR_HANDLE BindlessHeapStorage::GetUAVGpuBase() const
{
    return GetGpuHandle(BindlessHeapLayout::kUavBase);
}

D3D12_GPU_DESCRIPTOR_HANDLE BindlessHeapStorage::GetCBVGpuBase() const
{
    return GetGpuHandle(BindlessHeapLayout::kCbvBase);
}

}