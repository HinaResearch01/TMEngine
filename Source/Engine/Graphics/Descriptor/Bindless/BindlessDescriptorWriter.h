#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include "BindlessHeapStorage.h"
#include "BindlessSlotAllocator.h"

namespace tme::sys::graphics {

// CPU側DescriptorをGPU可視ヒープへコピーする責任だけを担う
// BindlessHeapStorageとindexを受け取って書き込む
class BindlessDescriptorWriter {

public:
    // 登録: srcHandleをheap上の指定スロットへコピーする
    static void WriteSRV(
        ID3D12Device* device,
        const BindlessHeapStorage& storage,
        uint32_t slotIndex,
        D3D12_CPU_DESCRIPTOR_HANDLE srcHandle);

    static void WriteUAV(
        ID3D12Device* device,
        const BindlessHeapStorage& storage,
        uint32_t slotIndex,
        D3D12_CPU_DESCRIPTOR_HANDLE srcHandle);

    static void WriteCBV(
        ID3D12Device* device,
        const BindlessHeapStorage& storage,
        uint32_t slotIndex,
        D3D12_CPU_DESCRIPTOR_HANDLE srcHandle);

private:
    static void WriteInternal(
        ID3D12Device* device,
        const BindlessHeapStorage& storage,
        uint32_t absoluteIndex,
        D3D12_CPU_DESCRIPTOR_HANDLE srcHandle);
};

}