#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <cstdint>

namespace tme::sys::graphics {

class CommandContext {

public:
	CommandContext() = default;
	~CommandContext() = default;

	// 初期化
	void Init(Microsoft::WRL::ComPtr<ID3D12Device> device,
		uint32_t numAllocators,
		D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

	// リセットして、再使用可能にする
	void Reset(uint32_t frameIndex, ID3D12PipelineState* prevPipelineState = nullptr);

	// コマンドの記録を終了する
	void Close();


	void SetDescriptorHeapsIfNeeded(ID3D12DescriptorHeap* cbvSrvUavHeap, ID3D12DescriptorHeap* samplerHeap = nullptr);

#pragma region Accessor
	ID3D12GraphicsCommandList* GetList() const { return commandList_.Get(); }
	ID3D12CommandAllocator* GetCurrentAllocator() const {
		if (currentAllocatorIndex_ < allocators_.size()) {
			return allocators_[currentAllocatorIndex_].Get();
		}
		return nullptr;
	}
#pragma endregion

private:
	// リスト
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
	// アロケータ（メモリの束、フレーム数分だけ持つ）
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> allocators_;
	// 現在開いているアロケータのインデックス
	uint32_t currentAllocatorIndex_ = 0;
	// 今どのヒープをバインドしているか
	ID3D12DescriptorHeap* currentBoundHeaps_[2] = { nullptr, nullptr };
};

}