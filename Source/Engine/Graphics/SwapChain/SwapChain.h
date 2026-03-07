#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <vector>
#include <cstdint>
#include "Engine/Graphics/Device/GraphicsDevice.h"
#include "Engine/Graphics/Cmd/CommandQueue.h"
#include "Engine/Graphics/Descriptor/PersistentDescriptorHeap.h"

namespace tme::sys::graphics {

class SwapChain {

public:
	// ダブルバッファリング用に2枚以上用意
	static constexpr uint32_t NUM_BACK_BUFFERS = 2;

public:
	SwapChain() = default;
	~SwapChain() = default;

	// 初期化
	void Init(GraphicsDevice* device, CommandQueue* commandQueue,
		HWND hwnd, uint32_t width, uint32_t height, PersistentDescriptorHeap* rtvHeap);

	// 裏表をフリップする
	void Present(bool vSync = true);

	// バッファサイズが変更されたとき
	void Resize(uint32_t width, uint32_t height);

private:
	void CreateRenderTargets();
	void ReleaseRenderTargets();

public:
#pragma region Accessor
	IDXGISwapChain4* GetSwapChain() const { return swapChain_.Get(); }
	ID3D12Resource* GetRenderTarget(uint32_t index) const { return renderTargets_[index].Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(uint32_t index) const { return rtvHandles_[index].cpuHandle; }

	// 今書き込むべきバックバッファのインデックス（0 か 1）
	uint32_t GetCurrentBackBufferIndex() const { return currentBackBufferIndex_; }

	// 現在のバックバッファのリソースとハンドルをサクッと取る用
	ID3D12Resource* GetCurrentRenderTarget() const { return renderTargets_[currentBackBufferIndex_].Get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVHandle() const { return rtvHandles_[currentBackBufferIndex_].cpuHandle; }

	// 画面フォーマットは標準的な sRGB 対応フォーマットにしておく(ポストエフェクトと相性が良いため)
	DXGI_FORMAT GetFormat() const { return DXGI_FORMAT_R8G8B8A8_UNORM; }
#pragma endregion

private:
	GraphicsDevice* device_ = nullptr;
	PersistentDescriptorHeap* rtvHeap_ = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
	Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets_[NUM_BACK_BUFFERS];

	DescriptorHandle rtvHandles_[NUM_BACK_BUFFERS]; // RTVの場所（CPU/GPU）を記録
	uint32_t currentBackBufferIndex_ = 0;
	uint32_t width_ = 0;
	uint32_t height_ = 0;
	HWND hwnd_ = nullptr;
};

}