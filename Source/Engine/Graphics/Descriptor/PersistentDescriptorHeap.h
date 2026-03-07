#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "DescriptorHandle.h" 

namespace tme::sys::graphics {

class PersistentDescriptorHeap {

public:
	PersistentDescriptorHeap() = default;
	~PersistentDescriptorHeap() = default;

	// ヒープの種類、収納できる最大数を指定して初期化
	void Init(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		uint32_t capacity
	);

	// 空いている住所を1つ貸し出す
	DescriptorHandle Allocate();

private:
	void CreateNullDescriptor();

public:
#pragma region Accessor
	ID3D12DescriptorHeap* GetHeap() const { return heap_.Get(); }
	D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return type_; }
	uint32_t GetDescriptorSize() const { return descriptorSize_; }
	// 空（無効）な場所を埋めるための Null Descriptor
	DescriptorHandle GetNullDescriptor() const { return nullDescriptor_; }
#pragma endregion

private:
	Microsoft::WRL::ComPtr<ID3D12Device> device_; // 引数で受け取るdevice
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
	D3D12_DESCRIPTOR_HEAP_TYPE type_ = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	uint32_t capacity_ = 0;
	uint32_t descriptorSize_ = 0;
	uint32_t currentOffset_ = 0;

	DescriptorHandle nullDescriptor_;
};

}