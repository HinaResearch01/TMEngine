#include "pch.h"
#include "PersistentDescriptorHeap.h"

namespace tme::sys::graphics {

void PersistentDescriptorHeap::Init(Microsoft::WRL::ComPtr<ID3D12Device> device,
	D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity)
{

	if (!device || capacity == 0) {
		LOG_ERROR("Graphics", "DescriptorAllocator::Init: Invalid arguments");
		return;
	}

	device_ = device;
	type_ = type;
	capacity_ = capacity;
	currentOffset_ = 0;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = type_;
	heapDesc.NumDescriptors = capacity_;
	// Persistent Heap の大原則: GPUからは見えない(保管用)設定にする
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;

	HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap_));
	if (FAILED(hr)) {
		LOG_ERROR("Graphics", "CreateDescriptorHeap failed in DescriptorAllocator");
		return;
	}

	descriptorSize_ = device->GetDescriptorHandleIncrementSize(type_);

	// 最初に1つだけNullDescriptor用として確保し、作成する
	nullDescriptor_ = this->Allocate();
	CreateNullDescriptor();

	LOG_INFO("Graphics", "DescriptorAllocator(Persistent) created", "type",
		(int)type_, "capacity", capacity_);
}

DescriptorHandle PersistentDescriptorHeap::Allocate()
{
	DescriptorHandle handle{};

	if (!heap_)
		return handle;

	if (currentOffset_ >= capacity_) {
		LOG_ERROR("Graphics", "DescriptorAllocator is FULL!!");
		return handle;
	}

	// CPU用の住所だけを計算する
	handle.cpuHandle = heap_->GetCPUDescriptorHandleForHeapStart();
	handle.cpuHandle.ptr += static_cast<SIZE_T>(currentOffset_) * descriptorSize_;

	currentOffset_++;

	return handle;
}

void PersistentDescriptorHeap::CreateNullDescriptor()
{
	if (!device_ || !nullDescriptor_.IsValid())
		return;

	// タイプに応じて空のディスクリプタを書き込む
	switch (type_) {
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV: {
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			device_->CreateShaderResourceView(nullptr, &srvDesc,
				nullDescriptor_.cpuHandle);
			break;
		}
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER: {
			D3D12_SAMPLER_DESC samplerDesc{};
			samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			device_->CreateSampler(&samplerDesc, nullDescriptor_.cpuHandle);
			break;
		}
		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		default:
			// RTVやDSVでNullを要求されることはほぼ無い
			break;
	}
}

}