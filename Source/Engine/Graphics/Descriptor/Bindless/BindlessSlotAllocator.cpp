#include "pch.h"
#include "BindlessSlotAllocator.h"

namespace tme::sys::graphics {

void BindlessSlotAllocator::Init(uint32_t capacity)
{
    if (capacity == 0) {
        LOG_ERROR("Graphics", "BindlessSlotAllocator::Init: capacity is 0");
        return;
    }
    capacity_ = capacity;
    nextOffset_ = 0;
    usedCount_ = 0;
    allocatedFlags_.assign(capacity, false);
}

uint32_t BindlessSlotAllocator::Allocate()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint32_t slotIndex = kInvalidDescriptorIndex;

    if (!freeList_.empty()) {
        slotIndex = freeList_.front();
        freeList_.pop();
    }
    else {
        if (nextOffset_ >= capacity_) {
            LOG_ERROR("Graphics", "BindlessSlotAllocator: Region is FULL",
                "capacity", capacity_);
            return kInvalidDescriptorIndex;
        }
        slotIndex = nextOffset_++;
    }

    allocatedFlags_[slotIndex] = true;
    ++usedCount_;
    return slotIndex;
}

void BindlessSlotAllocator::Release(uint32_t index, uint64_t retireFenceValue)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (index >= capacity_) {
        LOG_ERROR("Graphics", "BindlessSlotAllocator::Release: Index out of range",
            "index", index);
        return;
    }
    if (!allocatedFlags_[index]) {
        LOG_ERROR("Graphics", "BindlessSlotAllocator::Release: Double free detected",
            "index", index);
        return;
    }

    // 即時返却せず遅延キューへ
    allocatedFlags_[index] = false;
    --usedCount_;
    pendingFrees_.push({ index, retireFenceValue });
}

void BindlessSlotAllocator::ProcessDeferredFrees(uint64_t completedFenceValue)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    while (!pendingFrees_.empty()) {
        const PendingFreeEntry& entry = pendingFrees_.front();

        if (entry.retireFenceValue <= completedFenceValue) {
            freeList_.push(entry.index);
            pendingFrees_.pop();
        }
        else {
            // Fence値は単調増加を前提としているため、先頭が完了していなければ以降も未完了
            break;
        }
    }
}

bool BindlessSlotAllocator::IsAllocated(uint32_t index) const
{
    if (index >= capacity_)
        return false;
    return allocatedFlags_[index];
}

}