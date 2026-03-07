#include "pch.h"
#include "CommandContext.h"

namespace tme::sys::graphics {

void CommandContext::Init(Microsoft::WRL::ComPtr<ID3D12Device> device,
	uint32_t numAllocators, D3D12_COMMAND_LIST_TYPE type)
{
	if (!device || numAllocators == 0) {
		LOG_ERROR("Graphics", "CommandContext::Init: Invalid arguments");
		return;
	}

	// 指定された数（フレーム数）だけアロケータを作る
	allocators_.resize(numAllocators);
	for (uint32_t i = 0; i < numAllocators; ++i) {
		HRESULT hr =
			device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocators_[i]));
		if (FAILED(hr)) {
			LOG_ERROR("Graphics", "CreateCommandAllocator failed", "index", i);
			return;
		}
	}
	// 1ページ分のリストを作る。最初は[0]番目のアロケータを仮指定しておく。
	HRESULT hr = device->CreateCommandList(0, type, allocators_[0].Get(), nullptr,
		IID_PPV_ARGS(&commandList_));
	if (FAILED(hr)) {
		LOG_ERROR("Graphics", "CreateCommandList failed");
		return;
	}

	// 一旦閉じておく（最初に安全にResetできるように）
	commandList_->Close();
	currentAllocatorIndex_ = 0;
	currentBoundHeaps_[0] = nullptr;
	currentBoundHeaps_[1] = nullptr;

	LOG_INFO("Graphics", "CommandContext created with multiple allocators");
}

void CommandContext::Reset(uint32_t frameIndex,
	ID3D12PipelineState* prevPipelineState)
{
	if (allocators_.empty() || !commandList_)
		return;
	if (frameIndex >= allocators_.size()) {
		LOG_ERROR("Graphics", "CommandContext::Reset: frameIndex out of range");
		return;
	}

	// 現在のallocatorを取得
	currentAllocatorIndex_ = frameIndex;
	auto& currentAllocator = allocators_[currentAllocatorIndex_];

	// 今のフレーム用のアロケータを白紙にする
	HRESULT hr = currentAllocator->Reset();
	if (FAILED(hr)) {
		LOG_ERROR("Graphics", "CommandAllocator Reset failed", "frameIndex",
			frameIndex);
		return;
	}

	// これから記録するリストに、その白紙のアロケータを紐付ける
	hr = commandList_->Reset(currentAllocator.Get(), prevPipelineState);
	if (FAILED(hr)) {
		LOG_ERROR("Graphics", "CommandList Reset failed");
		return;
	}

	currentBoundHeaps_[0] = nullptr;
	currentBoundHeaps_[1] = nullptr;
}

void CommandContext::Close()
{
	if (commandList_) {
		HRESULT hr = commandList_->Close();
		if (FAILED(hr)) {
			LOG_ERROR("Graphics", "CommandList Close failed");
		}
	}
}

void CommandContext::SetDescriptorHeapsIfNeeded(ID3D12DescriptorHeap* cbvSrvUavHeap, ID3D12DescriptorHeap* samplerHeap)
{
	// 前回と全く同じヒープがバインドされているなら何もしない
	if (currentBoundHeaps_[0] == cbvSrvUavHeap &&
		currentBoundHeaps_[1] == samplerHeap) {
		return;
	}
	currentBoundHeaps_[0] = cbvSrvUavHeap;
	currentBoundHeaps_[1] = samplerHeap;

	ID3D12DescriptorHeap* heapsToBind[2] = {};
	uint32_t numHeaps = 0;

	if (cbvSrvUavHeap)
		heapsToBind[numHeaps++] = cbvSrvUavHeap;
	if (samplerHeap)
		heapsToBind[numHeaps++] = samplerHeap;

	if (numHeaps > 0) {
		commandList_->SetDescriptorHeaps(numHeaps, heapsToBind);
	}
}

}