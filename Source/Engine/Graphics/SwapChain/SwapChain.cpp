#include "pch.h"
#include "SwapChain.h"
#include "Engine/Graphics/Cmd/CommandQueue.h" 

namespace tme::sys::graphics {

void SwapChain::Init(GraphicsDevice* device, CommandQueue* commandQueue,
	HWND hwnd, uint32_t width, uint32_t height, PersistentDescriptorHeap* rtvHeap)
{
	if (!device || !commandQueue || !hwnd || !rtvHeap) {
		LOG_ERROR("Graphics", "SwapChain::Init: Invalid arguments");
		return;
	}
	device_ = device;
	hwnd_ = hwnd;
	width_ = width;
	height_ = height;
	rtvHeap_ = rtvHeap;

	// SwapChain の詳細設定
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = width_;
	swapChainDesc.Height = height_;
	swapChainDesc.Format = GetFormat(); // R8G8B8A8_UNORM
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = NUM_BACK_BUFFERS;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; // 可変リフレッシュレート対応用

	auto factory = device_->GetFactory();

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;

	// CommandQueueを渡してSwapChainを作成
	HRESULT hr = factory->CreateSwapChainForHwnd(
		commandQueue->GetQueue(),
		hwnd_,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1
	);

	if (FAILED(hr)) {
		LOG_ERROR("Graphics", "CreateSwapChainForHwnd failed");
		return;
	}

	// IDXGISwapChain4 へキャスト
	swapChain1.As(&swapChain_);
	// 現在のバックバッファ番号を取得
	currentBackBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();
	// RTVを作成
	CreateRenderTargets();

	LOG_INFO("Graphics", "SwapChain created", "width", width_, "height", height_);
}

void SwapChain::Present(bool)
{
	if (!swapChain_) return;
	// 垂直同期(VSync)を待たない場合は第一引数を0、待つ場合は1にする
	// 今回は可変フレームレートを考慮して一時的に0(即時反映)にしておく
	HRESULT hr = swapChain_->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	if (FAILED(hr)) {
		LOG_ERROR("Graphics", "SwapChain Present failed");
	}
	// フリップ後、次の書き込み対象バッファのインデックスを更新
	currentBackBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();
}

void SwapChain::Resize(uint32_t width, uint32_t height)
{
	if (!swapChain_ || (width_ == width && height_ == height)) return;

	width_ = width;
	height_ = height;

	// リサイズする前に、今のRenderTarget(バッファ)を解放しなければならない
	ReleaseRenderTargets();

	// SwapChainのバッファサイズを変更
	HRESULT hr = swapChain_->ResizeBuffers(
		NUM_BACK_BUFFERS,
		width_,
		height_,
		GetFormat(),
		DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
	);

	if (FAILED(hr)) {
		LOG_ERROR("Graphics", "SwapChain ResizeBuffers failed");
		return;
	}

	currentBackBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();

	// サイズ変更後、作り直す
	CreateRenderTargets();
}

void SwapChain::CreateRenderTargets()
{
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = GetFormat();
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (uint32_t i = 0; i < NUM_BACK_BUFFERS; ++i) {
		// SwapChainから i 番目のバッファを取り出す
		HRESULT hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&renderTargets_[i]));

		if (FAILED(hr)) {
			LOG_ERROR("Graphics", "SwapChain GetBuffer failed", "index", i);
			return;
		}

		// デバッグ用に名前をつけておく
		std::wstring name = L"SwapChain BackBuffer " + std::to_wstring(i);
		renderTargets_[i]->SetName(name.c_str());

		// PersistentDescriptorHeapから1つ場所をもらう
		if (!rtvHandles_[i].IsValid()) {
			rtvHandles_[i] = rtvHeap_->Allocate();
		}

		// RTVを作成してバッファの先頭に紐付ける
		device_->GetDevice()->CreateRenderTargetView(
			renderTargets_[i].Get(),
			&rtvDesc,
			rtvHandles_[i].cpuHandle
		);
	}
}

void SwapChain::ReleaseRenderTargets()
{
	for (uint32_t i = 0; i < NUM_BACK_BUFFERS; ++i) {
		// ComPtrのポインタをリセット
		renderTargets_[i].Reset();
	}
}

}