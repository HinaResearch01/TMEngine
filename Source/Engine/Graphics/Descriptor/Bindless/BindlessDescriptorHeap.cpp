#include "pch.h"
#include "BindlessDescriptorHeap.h"
#include "BindlessSlotAllocator.h"

namespace tme::sys::graphics {

void BindlessDescriptorHeap::Init(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
    if (!device) {
        LOG_ERROR("Graphics", "BindlessDescriptorHeap::Init: Device is null");
        return;
    }
    device_ = device;

    storage_.Init(device);

    srvAllocator_.Init(BindlessHeapLayout::kSrvCount);
    uavAllocator_.Init(BindlessHeapLayout::kUavCount);
    cbvAllocator_.Init(BindlessHeapLayout::kCbvCount);
}

BindlessHandle BindlessDescriptorHeap::RegisterSRV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle)
{
    return RegisterInternal(DescriptorKind::SRV, srvAllocator_,
        BindlessHeapLayout::kSrvBase, srcHandle);
}

BindlessHandle BindlessDescriptorHeap::RegisterUAV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle)
{
    return RegisterInternal(DescriptorKind::UAV, uavAllocator_,
        BindlessHeapLayout::kUavBase, srcHandle);
}

BindlessHandle BindlessDescriptorHeap::RegisterCBV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle)
{
    return RegisterInternal(DescriptorKind::CBV, cbvAllocator_,
        BindlessHeapLayout::kCbvBase, srcHandle);
}

BindlessHandle BindlessDescriptorHeap::RegisterInternal(
    DescriptorKind kind,
    BindlessSlotAllocator& allocator,
    uint32_t baseIndex,
    D3D12_CPU_DESCRIPTOR_HANDLE srcHandle)
{
    BindlessHandle result{ kind, kInvalidDescriptorIndex };

    if (!storage_.IsInitialized()) {
        LOG_ERROR("Graphics", "BindlessDescriptorHeap::Register: Not initialized");
        return result;
    }
    if (srcHandle.ptr == 0) {
        LOG_ERROR("Graphics", "BindlessDescriptorHeap::Register: srcHandle is null");
        return result;
    }

    const uint32_t slotIndex = allocator.Allocate();
    if (slotIndex == kInvalidDescriptorIndex)
        return result;

    // Writerがコピーする (Storage + slotIndex → 絶対indexを内部で計算)
    switch (kind) {
        case DescriptorKind::SRV:
            BindlessDescriptorWriter::WriteSRV(device_.Get(), storage_, slotIndex, srcHandle);
            break;
        case DescriptorKind::UAV:
            BindlessDescriptorWriter::WriteUAV(device_.Get(), storage_, slotIndex, srcHandle);
            break;
        case DescriptorKind::CBV:
            BindlessDescriptorWriter::WriteCBV(device_.Get(), storage_, slotIndex, srcHandle);
            break;
    }

    result.index = slotIndex;
    return result;
}

bool BindlessDescriptorHeap::Update(
    BindlessHandle handle, D3D12_CPU_DESCRIPTOR_HANDLE srcHandle)
{
    if (!handle.IsValid()) {
        LOG_ERROR("Graphics", "BindlessDescriptorHeap::Update: Invalid handle");
        return false;
    }
    if (srcHandle.ptr == 0) {
        LOG_ERROR("Graphics", "BindlessDescriptorHeap::Update: srcHandle is null");
        return false;
    }

    BindlessSlotAllocator& allocator = GetAllocator(handle.kind);
    if (!allocator.IsAllocated(handle.index)) {
        LOG_ERROR("Graphics", "BindlessDescriptorHeap::Update: Slot is not allocated",
            "index", handle.index);
        return false;
    }

    switch (handle.kind) {
        case DescriptorKind::SRV:
            BindlessDescriptorWriter::WriteSRV(device_.Get(), storage_, handle.index, srcHandle);
            break;
        case DescriptorKind::UAV:
            BindlessDescriptorWriter::WriteUAV(device_.Get(), storage_, handle.index, srcHandle);
            break;
        case DescriptorKind::CBV:
            BindlessDescriptorWriter::WriteCBV(device_.Get(), storage_, handle.index, srcHandle);
            break;
    }

    return true;
}

void BindlessDescriptorHeap::Release(BindlessHandle handle, uint64_t retireFenceValue)
{
    if (!handle.IsValid()) {
        LOG_ERROR("Graphics", "BindlessDescriptorHeap::Release: Invalid handle");
        return;
    }
    GetAllocator(handle.kind).Release(handle.index, retireFenceValue);
}

void BindlessDescriptorHeap::ProcessDeferredFrees(uint64_t completedFenceValue)
{
    srvAllocator_.ProcessDeferredFrees(completedFenceValue);
    uavAllocator_.ProcessDeferredFrees(completedFenceValue);
    cbvAllocator_.ProcessDeferredFrees(completedFenceValue);
}

BindlessSlotAllocator& BindlessDescriptorHeap::GetAllocator(DescriptorKind kind)
{
    switch (kind) {
        case DescriptorKind::UAV: return uavAllocator_;
        case DescriptorKind::CBV: return cbvAllocator_;
        default:                  return srvAllocator_;
    }
}

}