#include "pch.h"
#include "GraphicsDevice.h"

using Microsoft::WRL::ComPtr;
namespace tme::sys::graphics {

void GraphicsDevice::Init()
{
	HRESULT hr;
#ifdef _DEBUG
	hr = EnableDebaguLayer();
	if (FAILED(hr)) {
		LOG_WARN("Graphics", "Create DebugLayer failed");
	}
#endif //  _DEBUG

	hr = CreateFactory();
	if (FAILED(hr)) {
		LOG_ERROR("Graphics", "Create Factory/Adaptor failed");
	}

	hr = CreateDevice();
	if (FAILED(hr)) {
		LOG_ERROR("Graphics", "Create Device failed");
	}
}

HRESULT GraphicsDevice::EnableDebaguLayer()
{
	ComPtr<ID3D12Debug> debug;
	HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
	if (SUCCEEDED(hr) && debug) {
		debug->EnableDebugLayer();
		LOG_INFO("Graphics", "DebugLayer enabled");
	}
	return hr;
}

HRESULT GraphicsDevice::CreateFactory()
{
	factory_.Reset();
	useAdapter_.Reset();

	// 1. Factoryの作成
	UINT factoryFlags = 0;
#ifdef _DEBUG
	factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory_));
	if (FAILED(hr) || !factory_) {
		LOG_ERROR("Graphics", "CreateDXGIFactory2 failed",
			"hr", (unsigned)hr);
		return hr;
	}

	LOG_INFO("Graphics", "DXGI Factory created");

	// 2. Adapterの選択
	ComPtr<IDXGIFactory6> factory6;
	if (SUCCEEDED(factory_.As(&factory6))) {
		for (UINT i = 0;; ++i) {
			ComPtr<IDXGIAdapter1> adapter;
			hr = factory6->EnumAdapterByGpuPreference(i,
				DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
				IID_PPV_ARGS(&adapter));

			if (hr == DXGI_ERROR_NOT_FOUND) {
				LOG_ERROR("Graphics", "No High Performance hardware adapter found");
				break;
			}
			if (FAILED(hr)) {
				LOG_WARN("Graphics", "EnumAdapterByGpuPreference failed", "i:", i);
				continue;
			}

			DXGI_ADAPTER_DESC1 desc{};
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
				LOG_INFO("Graphics", "Skip software adapter");
				continue;
			}

			LOG_INFO("Graphics", "Use High Performance Adapter selected");
			adapter.As(&useAdapter_);
			return S_OK;
		}
	}
	else {
		// IDXGIFactory6が使えない環境のフォールバック
		for (UINT i = 0;; ++i) {
			ComPtr<IDXGIAdapter1> adapter;
			hr = factory_->EnumAdapters1(i, &adapter);
			if (hr == DXGI_ERROR_NOT_FOUND) {
				LOG_ERROR("Graphics", "No hardware adapter found");
				break;
			}
			if (FAILED(hr)) {
				LOG_WARN("Graphics", "EnumAdapters1 failed", "i:", i);
				continue;
			}

			DXGI_ADAPTER_DESC1 desc{};
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
				LOG_INFO("Graphics", "Skip software adapter");
				continue;
			}

			LOG_INFO("Graphics", "Use Adapter selected (Fallback)");
			adapter.As(&useAdapter_);
			return S_OK;
		}
	}

	return E_FAIL;
}

HRESULT GraphicsDevice::CreateDevice()
{
	if (!useAdapter_) {
		LOG_ERROR("Graphics", "CreateDevice: adapter is null");
		return E_POINTER;
	}

	static constexpr D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0
	};

	device_.Reset();

	for (auto fl : levels) {
		LOG_INFO("Graphics", "Try D3D12CreateDevice",
			"FL:", static_cast<int>(fl));

		HRESULT hr = D3D12CreateDevice(
			useAdapter_.Get(),
			fl,
			IID_PPV_ARGS(&device_)
		);

		if (SUCCEEDED(hr) && device_) {
			LOG_INFO("Graphics", "D3D12Device created", "FL:", static_cast<int>(fl));
			return S_OK;
		}

		LOG_WARN("Graphics", "D3D12CreateDevice failed",
			"FL:", static_cast<int>(fl),
			"hr:", static_cast<unsigned>(hr));
	}

	LOG_ERROR("Graphics", "All feature levels failed in D3D12CreateDevice");
	return E_FAIL;
}

}