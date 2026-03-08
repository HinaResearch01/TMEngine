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
    pendingFrees_.push_back({ index, retireFenceValue });
}

void BindlessSlotAllocator::ProcessDeferredFrees(uint64_t completedFenceValue)
{
    size_t writeIndex = 0;

    for (size_t readIndex = 0; readIndex < pendingFrees_.size(); ++readIndex) {
        const PendingFreeEntry& entry = pendingFrees_[readIndex];

        if (entry.retireFenceValue <= completedFenceValue) {
            freeList_.push(entry.index);
        }
        else {
            if (writeIndex != readIndex) {
                pendingFrees_[writeIndex] = pendingFrees_[readIndex];
            }
            ++writeIndex;
        }
    }

    pendingFrees_.resize(writeIndex);
}

bool BindlessSlotAllocator::IsAllocated(uint32_t index) const
{
    if (index >= capacity_)
        return false;
    return allocatedFlags_[index];
}

}