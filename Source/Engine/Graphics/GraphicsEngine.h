#pragma once

#include "Cmd/CommandContext.h"
#include "Cmd/CommandQueue.h"
#include "Descriptor/DynamicDescriptorHeap.h"
#include "Descriptor/PersistentDescriptorHeap.h"
#include "Descriptor/Bindless/BindlessDescriptorHeap.h"
#include "Device/GraphicsDevice.h"
#include "Shader/ShaderManager.h"
#include "SwapChain/SwapChain.h"
#include <memory>
#include <string>

namespace tme::sys::graphics {

class GraphicsEngine {

public:
	GraphicsEngine();
	~GraphicsEngine() = default;

	void Init(HWND hwnd, uint32_t width, uint32_t height);

	// -- Loop Sycle
	void BeginFrame();
	void EndFrame();

	void Shutdown();

#pragma region Accessor
	GraphicsDevice* GetDevice() const { return device_.get(); }
	CommandQueue* GetCommandQueue() const { return commandQueue_.get(); }
	CommandContext* GetCommandContext() const { return commandContext_.get(); }
	SwapChain* GetSwapChain() const { return swapChain_.get(); }
	// ディスクリプタヒープ
	PersistentDescriptorHeap* GetRtvHeap() const { return rtvHeap_.get(); }
	PersistentDescriptorHeap* GetDsvHeap() const { return dsvHeap_.get(); }
	PersistentDescriptorHeap* GetSrvCbvUavHeap() const { return srvCbvUavHeap_.get(); }
	DynamicDescriptorHeap* GetDynamicHeap() const { return dynamicHeap_.get(); }
	BindlessDescriptorHeap* GetBindlessHeap() const { return bindlessHeap_.get(); }
#pragma endregion

private:
	// --- コアコンポーネント ---
	std::unique_ptr<GraphicsDevice> device_;
	std::unique_ptr<CommandQueue> commandQueue_;
	std::unique_ptr<CommandContext> commandContext_;
	std::unique_ptr<SwapChain> swapChain_;

	// --- ディスクリプタヒープ ---
	// 保管庫 (Persistent)
	std::unique_ptr<PersistentDescriptorHeap> rtvHeap_;
	std::unique_ptr<PersistentDescriptorHeap> dsvHeap_;
	std::unique_ptr<PersistentDescriptorHeap> srvCbvUavHeap_;
	// 展示棚 (Dynamic)
	std::unique_ptr<DynamicDescriptorHeap> dynamicHeap_;
	// Bindless
	std::unique_ptr<BindlessDescriptorHeap> bindlessHeap_;

	// --- シェーダー ---
	std::unique_ptr<ShaderManager> shaderMgr_;

	// --- 状態管理 ---
	uint32_t currentFrameIndex_ = 0;
	bool isInitialized_ = false;
};

} // namespace tme::sys::graphics