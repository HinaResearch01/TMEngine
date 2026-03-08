#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace tme::sys::graphics {

// ヒープ内レイアウト: [SRV領域][UAV領域][CBV領域]
struct BindlessHeapLayout {
    static constexpr uint32_t kSrvBase = 0;
    static constexpr uint32_t kSrvCount = 65536;
    static constexpr uint32_t kUavBase = kSrvBase + kSrvCount; // 65536
    static constexpr uint32_t kUavCount = 4096;
    static constexpr uint32_t kCbvBase = kUavBase + kUavCount; // 69632
    static constexpr uint32_t kCbvCount = 4096;
    static constexpr uint32_t kTotalCount = kCbvBase + kCbvCount; // 73728
};

// GPU可視ヒープの生存管理とハンドル提供のみを担う
// ヒープの作成・破棄・アドレス計算がこのクラスの責任
class BindlessHeapStorage {

public:
    BindlessHeapStorage() = default;
    ~BindlessHeapStorage() = default;

    void Init(Microsoft::WRL::ComPtr<ID3D12Device> device);

    // CommandContextにバインドするヒープ
    ID3D12DescriptorHeap* GetHeap()     const { return heap_.Get(); }
    uint32_t GetDescriptorSize() const { return descriptorSize_; }

    // 絶対インデックスからCPU/GPUハンドルを計算して返す
    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t absoluteIndex) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t absoluteIndex) const;

    // RootSignatureのDescriptorTable用: リージョン先頭GPUハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGpuBase() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetUAVGpuBase() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetCBVGpuBase() const;

    bool IsInitialized() const { return heap_ != nullptr; }

private:
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
    uint32_t descriptorSize_ = 0;
};

}