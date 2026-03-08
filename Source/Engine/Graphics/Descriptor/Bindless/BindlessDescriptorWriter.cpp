#include "pch.h"
#include "BindlessDescriptorWriter.h"

namespace tme::sys::graphics {

void BindlessDescriptorWriter::WriteSRV(
    ID3D12Device* device,
    const BindlessHeapStorage& storage,
    uint32_t slotIndex,
    D3D12_CPU_DESCRIPTOR_HANDLE srcHandle)
{
    const uint32_t absoluteIndex = BindlessHeapLayout::kSrvBase + slotIndex;
    WriteInternal(device, storage, absoluteIndex, srcHandle);
}

void BindlessDescriptorWriter::WriteUAV(
    ID3D12Device* device,
    const BindlessHeapStorage& storage,
    uint32_t slotIndex,
    D3D12_CPU_DESCRIPTOR_HANDLE srcHandle)
{
    const uint32_t absoluteIndex = BindlessHeapLayout::kUavBase + slotIndex;
    WriteInternal(device, storage, absoluteIndex, srcHandle);
}

void BindlessDescriptorWriter::WriteCBV(
    ID3D12Device* device,
    const BindlessHeapStorage& storage,
    uint32_t slotIndex,
    D3D12_CPU_DESCRIPTOR_HANDLE srcHandle)
{
    const uint32_t absoluteIndex = BindlessHeapLayout::kCbvBase + slotIndex;
    WriteInternal(device, storage, absoluteIndex, srcHandle);
}

void BindlessDescriptorWriter::WriteInternal(
    ID3D12Device* device,
    const BindlessHeapStorage& storage,
    uint32_t absoluteIndex,
    D3D12_CPU_DESCRIPTOR_HANDLE srcHandle)
{
    D3D12_CPU_DESCRIPTOR_HANDLE destHandle = storage.GetCpuHandle(absoluteIndex);

    device->CopyDescriptorsSimple(
        1,
        destHandle,
        srcHandle,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

}