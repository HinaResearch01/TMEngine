#include "pch.h"
#include "GraphicsEngine.h"

namespace tme::sys::graphics {

GraphicsEngine::GraphicsEngine() {
	device_ = std::make_unique<GraphicsDevice>();
	commandQueue_ = std::make_unique<CommandQueue>();
	commandContext_ = std::make_unique<CommandContext>();
	rtvHeap_ = std::make_unique<PersistentDescriptorHeap>();
	dsvHeap_ = std::make_unique<PersistentDescriptorHeap>();
	srvCbvUavHeap_ = std::make_unique<PersistentDescriptorHeap>();
	dynamicHeap_ = std::make_unique<DynamicDescriptorHeap>();
	swapChain_ = std::make_unique<SwapChain>();
	shaderMgr_ = std::make_unique<ShaderManager>();
}

void GraphicsEngine::Init(HWND hwnd, uint32_t width, uint32_t height) {
	// Device の初期化
	device_->Init();

	// CommandQueue の初期化
	commandQueue_->Init(device_->GetDevice());

	// CommandContext の初期化 (フレーム数分のアロケータを作る)
	commandContext_->Init(device_->GetDevice(), NUM_FRAMES);

	// DescriptorHeap (保管庫) の初期化
	rtvHeap_->Init(device_->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 64);
	dsvHeap_->Init(device_->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 64);
	srvCbvUavHeap_->Init(device_->GetDevice(),
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);

	// DescriptorHeap (展示棚) の初期化
	dynamicHeap_->Init(device_->GetDevice(), 1024);

	// SwapChain の初期化
	swapChain_->Init(device_.get(), commandQueue_.get(), hwnd, width, height,
		rtvHeap_.get());

	// Shader の初期化
	shaderMgr_->Init();
	//shaderMgr_->LoadFromJSON(L"");

	isInitialized_ = true;
	LOG_INFO("Graphics", "GraphicsEngine initialization complete");
}

void GraphicsEngine::BeginFrame() {
	if (!isInitialized_)
		return;

	// 現在のフレームがCPUから触って安全か待機する
	commandQueue_->WaitForFenceValue(currentFrameIndex_);

	// DynamicHeapを空にする
	dynamicHeap_->Reset();

	// コマンドリストを、このフレーム用の白紙ノートにして記録開始
	commandContext_->Reset(currentFrameIndex_, nullptr);

	// ---- クリアーバッファーの設定
	auto cmdList = commandContext_->GetList();

	// 現在のバックバッファとRTVハンドルを取得
	auto currentBackBuffer = swapChain_->GetCurrentRenderTarget();
	auto currentRtvHandle = swapChain_->GetCurrentRTVHandle();
	// バリア：PRESENTからRENDER_TARGETへ状態遷移
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = currentBackBuffer;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	cmdList->ResourceBarrier(1, &barrier);
	// RTV に対してクリアを実行
	const float clearColor[4] = { 0.1f, 0.2f, 0.4f, 1.0f };
	cmdList->ClearRenderTargetView(currentRtvHandle, clearColor, 0, nullptr);

	// RTVをセット
	cmdList->OMSetRenderTargets(1, &currentRtvHandle, FALSE, nullptr);
}

void GraphicsEngine::EndFrame() {
	if (!isInitialized_)
		return;

	auto cmdList = commandContext_->GetList();
	auto currentBackBuffer = swapChain_->GetCurrentRenderTarget();
	// バリア：RENDER_TARGETからPRESENTへ戻す
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = currentBackBuffer;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	cmdList->ResourceBarrier(1, &barrier);

	// コマンドリストの記録を終了
	commandContext_->Close();

	// 記録したコマンドリストをコマンドキューに投げてGPUに実行させる
	ID3D12CommandList* ppCommandLists[] = { cmdList };
	commandQueue_->GetQueue()->ExecuteCommandLists(1, ppCommandLists);

	// 画面のフリップ
	swapChain_->Present();

	// シグナル番号を発行
	commandQueue_->Signal(currentFrameIndex_);

	// 次のフレームの準備
	currentFrameIndex_ = (currentFrameIndex_ + 1) % NUM_FRAMES;
}

void GraphicsEngine::Shutdown() {
	if (!isInitialized_)
		return;

	LOG_INFO("Graphics", "Terminating GraphicsEngine...");

	// すべてのリソースを安全に解放するために、GPUが現在の作業をすべて終えるまで待つ
	if (commandQueue_) {
		commandQueue_->WaitIdle();
	}

	// 各コンポーネントは std::unique_ptr により自動的に破棄される
	isInitialized_ = false;
	LOG_INFO("Graphics", "GraphicsEngine terminated");
}

} // namespace tme::sys::graphics