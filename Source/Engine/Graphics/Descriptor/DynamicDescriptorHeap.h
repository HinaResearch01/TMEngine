#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "DescriptorHandle.h"
#include "Engine/Graphics/Cmd/CommandContext.h" 
#include "PersistentDescriptorHeap.h"

namespace tme::sys::graphics {

class DynamicDescriptorHeap {

public:
	DynamicDescriptorHeap() = default;
	~DynamicDescriptorHeap() = default;

	// フレームの描画で使う最大数を指定して初期化
	void Init(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		uint32_t capacity);

	// フレーム開始時に呼ぶ。ヒープ空にする
	void Reset();

	// 必要なハンコをDescriptorHeapからこのdynamicにコピーし、
	// コマンドリストにヒープを使用白と命令する
	void CommitGraphicsDescriptors(
		CommandContext& cmdContext,
		uint32_t rootParameterIndex,
		const DescriptorHandle* srcHandles,
		uint32_t numDescriptors,
		const PersistentDescriptorHeap* persistentHeap
	);
	void CommitComputeDescriptors(
		CommandContext& cmdContext,
		uint32_t rootParameterIndex,
		const DescriptorHandle* srcHandles,
		uint32_t numDescriptors,
		const PersistentDescriptorHeap* persistentHeap
	);

private:
	// 新しいヒープをプールから取得、または新規作成する
	ID3D12DescriptorHeap* RequestNewHeap();

	// コピーとバインドの共通処理
	void CommitInternal(
		CommandContext& cmdContext,
		uint32_t rootParameterIndex,
		const DescriptorHandle* srcHandles,
		uint32_t numDescriptors,
		const PersistentDescriptorHeap* persistentHeap,
		bool isGraphics
	);

private:
	Microsoft::WRL::ComPtr<ID3D12Device> device_;
	uint32_t numDescriptorsPerPage_ = 0;
	uint32_t descriptorSize_ = 0;
	// ページング用のヒーププール
	std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> heapPool_;

	// 現在使っているヒープの番号と、その中の書き込み位置
	uint32_t currentHeapIndex_ = 0;
	uint32_t currentOffset_ = 0;

};

}