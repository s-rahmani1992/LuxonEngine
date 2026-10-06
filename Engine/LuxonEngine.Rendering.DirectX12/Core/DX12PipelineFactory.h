#pragma once
#include "pch.h"
#include "BasicTypes.h"
#include <string>

namespace LuxonEngine::Rendering {
	class Material;
	class ShaderProgram;
}

namespace LuxonEngine::Rendering::DX12 {
	namespace MeshShading {
		class DX12MeshPipelineModule;
		class HLSLMeshProgram;

		struct MeshPipelineProperties {
			bool enableDepthTest = true;
		};
	}

	namespace Rasterization {
		class DX12RasterizationPipelineModule;
		class HLSLRasterizationProgram;
		struct RasterizationPipelineProperties {
			bool enableDepthTest = true;
			bool indexBufferEnabled = true;
			D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // point, line or triangle. patches are not supported
		};
	}

	class DX12PipelineFactory {
	public:
		DX12PipelineFactory(ID3D12Device10* device);

		ref<MeshShading::DX12MeshPipelineModule> CreateMeshPipeline(Material* material, const MeshShading::MeshPipelineProperties& properties, std::string& error);
		ref<MeshShading::DX12MeshPipelineModule> CreateMeshPipeline(const ShaderProgram* program, const MeshShading::MeshPipelineProperties& properties, std::string& error);

		ref<Rasterization::DX12RasterizationPipelineModule> CreateRasterizationPipeline(Material* material, const Rasterization::RasterizationPipelineProperties& properties, std::string& error);
		ref<Rasterization::DX12RasterizationPipelineModule> CreateRasterizationPipeline(const ShaderProgram* program, const Rasterization::RasterizationPipelineProperties& properties, std::string& error);

	private:
		ref<MeshShading::DX12MeshPipelineModule> CreateMeshPipeline(const ShaderProgram* program, Material* material, const MeshShading::MeshPipelineProperties& properties, std::string& error);

		ref<Rasterization::DX12RasterizationPipelineModule> CreateRasterizationPipeline(const ShaderProgram* program, Material* material, const Rasterization::RasterizationPipelineProperties& properties, std::string& error);

		template <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type, typename T>
		struct alignas(void*) PsoSubobject
		{
			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type = Type;
			T value{};
		};

		struct MeshPsoStream
		{
			PsoSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*>     RootSignature;
			PsoSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS, D3D12_SHADER_BYTECODE>    AS;
			PsoSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS, D3D12_SHADER_BYTECODE>    MS;
			PsoSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE>    PS;
			PsoSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC>         Blend;
			PsoSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK, UINT>                     SampleMask;
			PsoSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, D3D12_RASTERIZER_DESC>    Rasterizer;
			PsoSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL, D3D12_DEPTH_STENCIL_DESC> DepthStencil;
			PsoSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY>    RTFormats;
			PsoSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT>              DSFormat;
			PsoSubobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DXGI_SAMPLE_DESC>         SampleDesc;
		};

	private:
		ID3D12Device10* m_device;
	};
}
