#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

namespace tme::sys::graphics {

class GraphicsDevice {

public:
	GraphicsDevice() = default;
	~GraphicsDevice() = default;

	void Init();

private:
	// (開発中のみ)DebaguLayerをオンにする
	HRESULT EnableDebaguLayer();
	// ファクトリー作成
	HRESULT CreateFactory();
	// デバイスの作成
	HRESULT CreateDevice();

public:
#pragma region Accessor
	ID3D12Device* GetDevice() const { return device_.Get(); }
	IDXGIFactory7* GetFactory() const { return factory_.Get(); }
#pragma endregion

private:
	Microsoft::WRL::ComPtr<IDXGIFactory7> factory_;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> useAdapter_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;
};

}