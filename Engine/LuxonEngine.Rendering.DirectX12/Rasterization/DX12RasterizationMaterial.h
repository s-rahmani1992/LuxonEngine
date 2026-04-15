#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include <vector>
#include <string>

using namespace Microsoft::WRL;

namespace QuantumEngine {
	namespace Rendering {
		class ShaderProgram;
		class Material;
	}
}

namespace QuantumEngine::Rendering::DX12::Rasterization {
	class HLSLRasterizationProgram;

	class DX12RasterizationMaterial {
	public:
		DX12RasterizationMaterial(const ref<Material>& material, const ref<HLSLRasterizationProgram>& program);

		void BindDescriptorToResources(const ComPtr<ID3D12DescriptorHeap>& descriptorHeap, UInt32 offset);
		void BindParameters(ComPtr<ID3D12GraphicsCommandList7>& commandList);
		void BindTransformDescriptor(ComPtr<ID3D12GraphicsCommandList7>& commandList, const D3D12_GPU_DESCRIPTOR_HANDLE& transformHeapHandle);
		void BindCameraDescriptor(ComPtr<ID3D12GraphicsCommandList7>& commandList, const D3D12_GPU_DESCRIPTOR_HANDLE& cameraHeapHandle);
		void BindLightDescriptor(ComPtr<ID3D12GraphicsCommandList7>& commandList, const D3D12_GPU_DESCRIPTOR_HANDLE& lightHeapHandle);
		void SetDescriptorHandles(const std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE& handle);
		inline ref<Material> GetMaterial() const { return m_material; }
		inline ref<Rasterization::HLSLRasterizationProgram> GetProgram() const { return m_program; }

	private:
		struct constantBufferData {
			UInt32 rootParamIndex;
			Byte* location;
			UInt32 size;
		};

		struct HeapData {
			UInt32 rootParamIndex;
			std::string name;
			D3D12_CPU_DESCRIPTOR_HANDLE originalCpuHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = 0 };
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = 0 };
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = D3D12_GPU_DESCRIPTOR_HANDLE{ .ptr = 0 };
		};

		ref<Material> m_material;
		ref<DX12::Rasterization::HLSLRasterizationProgram> m_program;
		std::vector<constantBufferData> m_constantRegisterValues;
		std::vector<HeapData> m_heapValues;
		ComPtr<ID3D12Device> m_device;
		UInt32 m_transformRootIndex = -1;
		UInt32 m_cameraRootIndex = -1;
		UInt32 m_lightRootIndex = -1;
	};
}