#include "pch.h"
#include "CommandQueue.h"

namespace tme::sys::graphics {

CommandQueue::~CommandQueue() {
	WaitIdle();
	if (fenceEvent_) {
		CloseHandle(fenceEvent_);
		fenceEvent_ = nullptr;
	}
}

void CommandQueue::Init(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type)
{
	if (!device) {
		LOG_ERROR("Graphics", "CommandQueue::Init: Device is null");
		return;
	}

	// キューの作成
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = type;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;
	HRESULT hr =
		device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_));
	if (FAILED(hr))
		return;

	// フェンスの作成
	hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	if (FAILED(hr))
		return;
	fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!fenceEvent_)
		return;

	// 各フレーム用のフェンス目標値を0で初期化
	frameFenceValues_.resize(NUM_FRAMES, 0);

	// 次に送るシグナルの番号は1からスタート
	nextFenceValue_ = 1;
}

uint64_t CommandQueue::Signal(uint32_t frameIndex)
{
	if (!commandQueue_ || !fence_ || frameIndex >= frameFenceValues_.size())
		return 0;

	// 今から発行するシグナルの番号を記憶しておく
	uint64_t signalValue = nextFenceValue_;

	// キューにシグナル命令を積む
	commandQueue_->Signal(fence_.Get(), signalValue);

	// そのフレームインデックス用の目標値として記録
	frameFenceValues_[frameIndex] = signalValue;

	// 次のシグナルのために番号を+1しておく
	nextFenceValue_++;

	return signalValue;
}

void CommandQueue::WaitForFenceValue(uint32_t frameIndex)
{
	if (!fence_ || !fenceEvent_ || frameIndex >= frameFenceValues_.size())
		return;

	// このフレームが待つべき目標値
	uint64_t targetValue = frameFenceValues_[frameIndex];

	// まだGPUの処理が targetValue に達していない場合だけ、イベントを作って待つ
	if (fence_->GetCompletedValue() < targetValue) {
		HRESULT hr = fence_->SetEventOnCompletion(targetValue, fenceEvent_);
		if (SUCCEEDED(hr)) {
			WaitForSingleObject(fenceEvent_, INFINITE);
		}
	}
}

void CommandQueue::WaitIdle()
{
	if (!commandQueue_ || !fence_ || !fenceEvent_)
		return;

	// 単純に「今ある最新の番号」までのシグナルを送る
	uint64_t signalValue = nextFenceValue_++;
	commandQueue_->Signal(fence_.Get(), signalValue);

	// その番号まで確実に待つ
	if (fence_->GetCompletedValue() < signalValue) {
		if (SUCCEEDED(fence_->SetEventOnCompletion(signalValue, fenceEvent_))) {
			WaitForSingleObject(fenceEvent_, INFINITE);
		}
	}
}

}