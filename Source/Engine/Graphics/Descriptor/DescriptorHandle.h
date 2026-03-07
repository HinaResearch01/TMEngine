#pragma once

#include <d3d12.h>

namespace tme::sys::graphics {

struct DescriptorHandle {
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{ 0 };
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{ 0 };
	bool IsValid() const { return cpuHandle.ptr != 0; }
	bool IsShaderVisible() const { return gpuHandle.ptr != 0; }
};

}