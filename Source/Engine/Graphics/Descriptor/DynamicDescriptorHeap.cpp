#include "pch.h"
#include "DynamicDescriptorHeap.h"

namespace tme::sys::graphics {

void DynamicDescriptorHeap::Init(Microsoft::WRL::ComPtr<ID3D12Device> device, uint32_t capacity)
{
	if (!device || capacity == 0) {
		LOG_ERROR("Graphics", "DynamicDescriptorHeap::Init: Invalid arguments");
		return;
	}

	device_ = device;
	numDescriptorsPerPage_ = capacity;
	currentOffset_ = 0;
	currentHeapIndex_ = 0;

	descriptorSize_ = device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 最初のヒープをプールに作っておく
	RequestNewHeap();

	LOG_INFO("Graphics", "DynamicDescriptorHeap created", "capacity",
		numDescriptorsPerPage_);
}

void DynamicDescriptorHeap::Reset()
{
	currentHeapIndex_ = 0;
	currentOffset_ = 0;
}

void DynamicDescriptorHeap::CommitGraphicsDescriptors(
	CommandContext& cmdContext, uint32_t rootParameterIndex,
	const DescriptorHandle* srcHandles, uint32_t numDescriptors,
	const PersistentDescriptorHeap* persistentHeap)
{
	CommitInternal(cmdContext, rootParameterIndex, srcHandles, numDescriptors,
		persistentHeap, true);
}

void DynamicDescriptorHeap::CommitComputeDescriptors(
	CommandContext& cmdContext, uint32_t rootParameterIndex,
	const DescriptorHandle* srcHandles, uint32_t numDescriptors,
	const PersistentDescriptorHeap* persistentHeap)
{
	CommitInternal(cmdContext, rootParameterIndex, srcHandles, numDescriptors,
		persistentHeap, false);
}

ID3D12DescriptorHeap* DynamicDescriptorHeap::RequestNewHeap()
{
	// プールに足りなければ新しく作って追加する
	if (currentHeapIndex_ >= heapPool_.size()) {
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.NumDescriptors = numDescriptorsPerPage_;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NodeMask = 0;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> newHeap;
		device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&newHeap));
		heapPool_.push_back(newHeap);
	}
	return heapPool_[currentHeapIndex_].Get();
}

void DynamicDescriptorHeap::CommitInternal(
	CommandContext& cmdContext, uint32_t rootParameterIndex,
	const DescriptorHandle* srcHandles, uint32_t numDescriptors,
	const PersistentDescriptorHeap* persistentHeap, bool isGraphics)
{
	if (numDescriptors == 0)
		return;
	// ページング処理
	if (currentOffset_ + numDescriptors > numDescriptorsPerPage_) {
		currentHeapIndex_++;
		currentOffset_ = 0;
	}

	ID3D12DescriptorHeap* currentHeap =
		RequestNewHeap(); // プールからヒープを取得/作成

	cmdContext.SetDescriptorHeapsIfNeeded(currentHeap, nullptr);

	// バッチコピーのための配列準備
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srcCpu(numDescriptors);
	std::vector<UINT> srcSizes(numDescriptors, 1); // 各srcは1個ずつ
	DescriptorHandle nullHandle = persistentHeap->GetNullDescriptor();
	for (uint32_t i = 0; i < numDescriptors; ++i) {
		if (srcHandles[i].IsValid()) {
			srcCpu[i] = srcHandles[i].cpuHandle;
		}
		else {
			// 無効なハンドルはゴミを掴まないように確実にNullで埋める
			srcCpu[i] = nullHandle.cpuHandle;
		}
	}

	// コピー先の起点を計算
	D3D12_CPU_DESCRIPTOR_HANDLE destHandleStart =
		currentHeap->GetCPUDescriptorHandleForHeapStart();
	destHandleStart.ptr += static_cast<SIZE_T>(currentOffset_) * descriptorSize_;

	UINT destRangeSize = numDescriptors;

	// 一発コピー
	device_->CopyDescriptors(1, &destHandleStart,
		&destRangeSize, // Dest側 (ひとまとまりの連続配列)
		numDescriptors, srcCpu.data(),
		srcSizes.data(), // Src側 (バラバラのポインタの配列)
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// RootTable にセット
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandleStart =
		currentHeap->GetGPUDescriptorHandleForHeapStart();
	gpuHandleStart.ptr += static_cast<UINT64>(currentOffset_) * descriptorSize_;

	if (isGraphics) {
		cmdContext.GetList()->SetGraphicsRootDescriptorTable(rootParameterIndex,
			gpuHandleStart);
	}
	else {
		cmdContext.GetList()->SetComputeRootDescriptorTable(rootParameterIndex,
			gpuHandleStart);
	}

	// 次回のためにオフセットを進める
	currentOffset_ += numDescriptors;
}

}