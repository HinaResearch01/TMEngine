#pragma once

#include <cstdint>
#include <d3d12.h>
#include <wrl.h>
#include <vector>

namespace tme::sys::graphics {

// フレーム数
constexpr uint32_t NUM_FRAMES = 2;

class CommandQueue {

public:
	CommandQueue() = default;
	~CommandQueue();

	// 初期化
	void Init(Microsoft::WRL::ComPtr<ID3D12Device> device,
		D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

	// フェンス値を更新させるシグナルを送る
	uint64_t Signal(uint32_t frameIndex);

	// gpuが処理を追えているか待つ
	void WaitForFenceValue(uint32_t frameIndex);

	// アプリ初期化時や終了時に、gpuが全ての処理を終わらせるのを待つ
	void WaitIdle();

#pragma region Accessor
	ID3D12CommandQueue* GetQueue() const { return commandQueue_.Get(); }
#pragma endregion

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
	// フェンスは1枚で共通
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	HANDLE fenceEvent_ = nullptr;
	// 現在発行済みの最新のフェンス目標値
	uint64_t nextFenceValue_ = 1;
	// 各フレームごとのフェンス目標値を記憶しておく配列
	std::vector<uint64_t> frameFenceValues_;
};

}