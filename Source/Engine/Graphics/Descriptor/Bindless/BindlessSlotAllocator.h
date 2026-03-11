#pragma once

#include <cstdint>
#include <queue>
#include <vector>
#include <mutex>

#include "BindlessHeapStorage.h"

namespace tme::sys::graphics {

// 無効なDescriptorIndexを表す定数
static constexpr uint32_t kInvalidDescriptorIndex = UINT32_MAX;

enum class DescriptorKind : uint8_t {
    SRV, UAV, CBV
};

struct BindlessHandle {
    DescriptorKind kind = DescriptorKind::SRV;
    uint32_t       index = kInvalidDescriptorIndex;
    bool IsValid() const { return index != kInvalidDescriptorIndex; }
    
    // Shader Model 6.6のbindless用に絶対インデックスを取得する
    uint32_t GetAbsoluteIndex() const {
        if (!IsValid()) return kInvalidDescriptorIndex;
        switch (kind) {
            case DescriptorKind::SRV: return BindlessHeapLayout::kSrvBase + index;
            case DescriptorKind::UAV: return BindlessHeapLayout::kUavBase + index;
            case DescriptorKind::CBV: return BindlessHeapLayout::kCbvBase + index;
        }
        return kInvalidDescriptorIndex;
    }
};

// 1リージョン分のスロット管理を担う
// index確保・解放・遅延free・二重解放検出がこのクラスの責任
class BindlessSlotAllocator {

public:
	BindlessSlotAllocator() = default;
	~BindlessSlotAllocator() = default;

    struct PendingFreeEntry {
        uint32_t index = kInvalidDescriptorIndex;
        uint64_t retireFenceValue = 0;
    };

    void Init(uint32_t capacity);

    // スロット確保: Base相対のindexを返す。失敗時はkInvalidDescriptorIndex
    uint32_t Allocate();

    // 遅延解放: fenceが完了するまでfree listへ戻さない
    void Release(uint32_t index, uint64_t retireFenceValue);

    // 完了済みfence値を受け取り、解放可能なスロットをfree listへ戻す
    void ProcessDeferredFrees(uint64_t completedFenceValue);

    bool IsAllocated(uint32_t index) const;
    uint32_t GetCapacity()    const { return capacity_; }
    uint32_t GetUsedCount()   const { return usedCount_; }

private:
    uint32_t capacity_ = 0;
    uint32_t nextOffset_ = 0; // 一度も使っていない領域の先頭
    uint32_t usedCount_ = 0;

    std::queue<uint32_t> freeList_;
    std::queue<PendingFreeEntry> pendingFrees_;

    // Thread safety
    std::mutex mutex_;

    // デバッグ用: 二重解放検出
    std::vector<bool> allocatedFlags_;
};

}

