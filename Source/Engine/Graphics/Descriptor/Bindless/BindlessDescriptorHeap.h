#pragma once

#include "BindlessHeapStorage.h"
#include "BindlessSlotAllocator.h"
#include "BindlessDescriptorWriter.h"

namespace tme::sys::graphics {

// 3つのサブシステムを束ねるFacade
class BindlessDescriptorHeap {

public:
	BindlessDescriptorHeap() = default;
	~BindlessDescriptorHeap() = default;

	void Init(Microsoft::WRL::ComPtr<ID3D12Device> device);

	// CPU側のDescriptorHandleを受け取り、GPUヒープへコピーしてHandleを返す
	BindlessHandle RegisterSRV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle);
	BindlessHandle RegisterUAV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle);
	BindlessHandle RegisterCBV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle);

	// 既存スロットのDescriptorだけ更新する
	bool Update(BindlessHandle handle, D3D12_CPU_DESCRIPTOR_HANDLE srcHandle);

	// 遅延解放: GPUが使い終わるfence値まで返却を遅らせる
	void Release(BindlessHandle handle, uint64_t retireFenceValue);

	// 完了済みfence値を受け取り、解放可能なスロットをfree listへ戻す
	void ProcessDeferredFrees(uint64_t completedFenceValue);

	// CommandContextにバインドするヒープ取得
	ID3D12DescriptorHeap* GetHeap() const { return storage_.GetHeap(); }

	// RootSignatureのDescriptorTable用GPUハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGpuBase() const { return storage_.GetSRVGpuBase(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetUAVGpuBase() const { return storage_.GetUAVGpuBase(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetCBVGpuBase() const { return storage_.GetCBVGpuBase(); }

private:
	BindlessHandle RegisterInternal(
		DescriptorKind kind,
		BindlessSlotAllocator& allocator,
		uint32_t baseIndex,
		D3D12_CPU_DESCRIPTOR_HANDLE srcHandle);

	BindlessSlotAllocator& GetAllocator(DescriptorKind kind);

	Microsoft::WRL::ComPtr<ID3D12Device> device_;

	BindlessHeapStorage   storage_;
	BindlessSlotAllocator srvAllocator_;
	BindlessSlotAllocator uavAllocator_;
	BindlessSlotAllocator cbvAllocator_;
};

}