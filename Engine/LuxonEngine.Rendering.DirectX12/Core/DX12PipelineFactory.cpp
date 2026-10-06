#include "pch.h"
#include "DX12PipelineFactory.h"
#include "../Mesh/DX12MeshPipelineModule.h"
#include "../Mesh/HLSLMeshProgram.h"
#include "../Rasterization/DX12RasterizationPipelineModule.h"
#include "../Rasterization/HLSLRasterizationProgram.h"
#include "Rendering/Material.h"
#include <vector>

namespace LuxonEngine::Rendering::DX12 {
	DX12PipelineFactory::DX12PipelineFactory(ID3D12Device10* device)
		:m_device(device)
	{

	}

	ref<MeshShading::DX12MeshPipelineModule> DX12PipelineFactory::CreateMeshPipeline(Material* material, const MeshShading::MeshPipelineProperties& properties, std::string& error)
	{
		if (material == nullptr) {
			error = "Material is null";
			return nullptr;
		}

		return CreateMeshPipeline(material->GetProgram().get(), material, properties, error);
	}

	ref<MeshShading::DX12MeshPipelineModule> DX12PipelineFactory::CreateMeshPipeline(const ShaderProgram* program, const MeshShading::MeshPipelineProperties& properties, std::string& error)
	{
		return CreateMeshPipeline(program, nullptr, properties, error);
	}

	ref<MeshShading::DX12MeshPipelineModule> DX12PipelineFactory::CreateMeshPipeline(const ShaderProgram* program, Material* material, const MeshShading::MeshPipelineProperties& properties, std::string& error)
	{
		auto meshProgram = dynamic_cast<const MeshShading::HLSLMeshProgram*>(program);

		if (meshProgram == nullptr) {
			error = "Program is not a mesh shader program";
			return nullptr;
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
		if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7)))
			|| options7.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED) {
			error = "Mesh shaders are not supported by the device";
			return nullptr;
		}

		MeshPsoStream stream;

		stream.RootSignature.value = meshProgram->GetRootSignature().Get();
		stream.AS.value = meshProgram->GetAmplificationShaderBytecode(); // empty bytecode if the program has no amplification stage
		stream.MS.value = meshProgram->GetMeshShaderBytecode();
		stream.PS.value = meshProgram->GetPixelShaderBytecode();

		//Blend
		stream.Blend.value.AlphaToCoverageEnable = FALSE;
		stream.Blend.value.IndependentBlendEnable = FALSE;

		for (auto& renderTarget : stream.Blend.value.RenderTarget) {
			renderTarget.BlendEnable = FALSE;
			renderTarget.LogicOpEnable = FALSE;
			renderTarget.SrcBlend = D3D12_BLEND_ONE;
			renderTarget.DestBlend = D3D12_BLEND_ZERO;
			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			renderTarget.SrcBlendAlpha = D3D12_BLEND_ONE;
			renderTarget.DestBlendAlpha = D3D12_BLEND_ZERO;
			renderTarget.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			renderTarget.LogicOp = D3D12_LOGIC_OP_NOOP;
			renderTarget.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		}

		stream.SampleMask.value = 0xFFFFFFFF;

		//Rasterizer, same as the other rasterization pipelines of the engine
		stream.Rasterizer.value = D3D12_RASTERIZER_DESC{
			.FillMode = D3D12_FILL_MODE_SOLID,
			.CullMode = D3D12_CULL_MODE_FRONT,
			.FrontCounterClockwise = FALSE,
			.DepthBias = 0,
			.DepthBiasClamp = 0.0f,
			.SlopeScaledDepthBias = 0.0f,
			.DepthClipEnable = FALSE,
			.MultisampleEnable = FALSE,
			.AntialiasedLineEnable = FALSE,
			.ForcedSampleCount = 0,
			.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
		};

		//Depth buffer
		D3D12_DEPTH_STENCILOP_DESC stencilOp{
			.StencilFailOp = D3D12_STENCIL_OP_KEEP,
			.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
			.StencilPassOp = D3D12_STENCIL_OP_KEEP,
			.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS,
		};

		stream.DepthStencil.value = D3D12_DEPTH_STENCIL_DESC{
			.DepthEnable = properties.enableDepthTest ? TRUE : FALSE,
			.DepthWriteMask = properties.enableDepthTest ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO,
			.DepthFunc = D3D12_COMPARISON_FUNC_LESS,
			.StencilEnable = FALSE,
			.StencilReadMask = 0,
			.StencilWriteMask = 0,
			.FrontFace = stencilOp,
			.BackFace = stencilOp,
		};

		//RTVs, from the render target reflection of the pixel shader
		stream.RTFormats.value = meshProgram->GetRenderTargetFormats();

		//DSV
		stream.DSFormat.value = properties.enableDepthTest ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_UNKNOWN;

		//Sampling
		stream.SampleDesc.value = DXGI_SAMPLE_DESC{
			.Count = 1,
			.Quality = 0,
		};

		D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{
			.SizeInBytes = sizeof(stream),
			.pPipelineStateSubobjectStream = &stream,
		};

		ComPtr<ID3D12PipelineState> pipelineState;

		if (FAILED(m_device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipelineState)))) {
			error = "Failed to create mesh pipeline state";
			return nullptr;
		}

		auto pipeline = std::make_shared<MeshShading::DX12MeshPipelineModule>(m_device, pipelineState, meshProgram->GetRootSignature().Get(), material);

		return pipeline;
	}

	ref<Rasterization::DX12RasterizationPipelineModule> DX12PipelineFactory::CreateRasterizationPipeline(Material* material, const Rasterization::RasterizationPipelineProperties& properties, std::string& error)
	{
		if (material == nullptr) {
			error = "Material is null";
			return nullptr;
		}

		return CreateRasterizationPipeline(material->GetProgram().get(), material, properties, error);
	}

	ref<Rasterization::DX12RasterizationPipelineModule> DX12PipelineFactory::CreateRasterizationPipeline(const ShaderProgram* program, const Rasterization::RasterizationPipelineProperties& properties, std::string& error)
	{
		return CreateRasterizationPipeline(program, nullptr, properties, error);
	}

	ref<Rasterization::DX12RasterizationPipelineModule> DX12PipelineFactory::CreateRasterizationPipeline(const ShaderProgram* program, Material* material, const Rasterization::RasterizationPipelineProperties& properties, std::string& error)
	{
		auto rasterizationProgram = dynamic_cast<const Rasterization::HLSLRasterizationProgram*>(program);

		if (rasterizationProgram == nullptr) {
			error = "Program is not a rasterization shader program";
			return nullptr;
		}

		if (properties.topologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT
			&& properties.topologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE
			&& properties.topologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE) {
			error = "Unsupported primitive topology type of the rasterization pipeline";
			return nullptr;
		}

		IDxcBlob* vertexShader = rasterizationProgram->GetVertexShader();
		IDxcBlob* pixelShader = rasterizationProgram->GetPixelShader();
		IDxcBlob* geometryShader = rasterizationProgram->GetGeometryShader();

		if (vertexShader == nullptr) {
			error = "Rasterization program has no vertex shader";
			return nullptr;
		}

		ID3D12RootSignature* rootSignature = rasterizationProgram->GetRootSignature().Get();

		if (rootSignature == nullptr) {
			error = "Rasterization program has no root signature";
			return nullptr;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};

		//Input part, from the input signature of the vertex shader
		auto& inputElements = rasterizationProgram->GetInputAssemblyReflection().inputElements;

		pipelineStateDesc.InputLayout = D3D12_INPUT_LAYOUT_DESC{
			.pInputElementDescs = inputElements.empty() ? nullptr : inputElements.data(),
			.NumElements = (UINT)inputElements.size(),
		};
		pipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		pipelineStateDesc.PrimitiveTopologyType = properties.topologyType;

		//Shader part
		pipelineStateDesc.pRootSignature = rootSignature;
		pipelineStateDesc.VS = D3D12_SHADER_BYTECODE{ vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };

		if (pixelShader != nullptr)
			pipelineStateDesc.PS = D3D12_SHADER_BYTECODE{ pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };

		if (geometryShader != nullptr)
			pipelineStateDesc.GS = D3D12_SHADER_BYTECODE{ geometryShader->GetBufferPointer(), geometryShader->GetBufferSize() };

		//Rasterizer, same as the other rasterization pipelines of the engine
		pipelineStateDesc.RasterizerState = D3D12_RASTERIZER_DESC{
			.FillMode = D3D12_FILL_MODE_SOLID,
			.CullMode = D3D12_CULL_MODE_FRONT,
			.FrontCounterClockwise = FALSE,
			.DepthBias = 0,
			.DepthBiasClamp = 0.0f,
			.SlopeScaledDepthBias = 0.0f,
			.DepthClipEnable = FALSE,
			.MultisampleEnable = FALSE,
			.AntialiasedLineEnable = FALSE,
			.ForcedSampleCount = 0,
			.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
		};

		//Blend
		pipelineStateDesc.BlendState.AlphaToCoverageEnable = FALSE;
		pipelineStateDesc.BlendState.IndependentBlendEnable = FALSE;

		for (auto& renderTarget : pipelineStateDesc.BlendState.RenderTarget) {
			renderTarget.BlendEnable = FALSE;
			renderTarget.LogicOpEnable = FALSE;
			renderTarget.SrcBlend = D3D12_BLEND_ONE;
			renderTarget.DestBlend = D3D12_BLEND_ZERO;
			renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
			renderTarget.SrcBlendAlpha = D3D12_BLEND_ONE;
			renderTarget.DestBlendAlpha = D3D12_BLEND_ZERO;
			renderTarget.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			renderTarget.LogicOp = D3D12_LOGIC_OP_NOOP;
			renderTarget.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		}

		pipelineStateDesc.SampleMask = 0xFFFFFFFF;

		//Depth buffer
		D3D12_DEPTH_STENCILOP_DESC stencilOp{
			.StencilFailOp = D3D12_STENCIL_OP_KEEP,
			.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
			.StencilPassOp = D3D12_STENCIL_OP_KEEP,
			.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS,
		};

		pipelineStateDesc.DepthStencilState = D3D12_DEPTH_STENCIL_DESC{
			.DepthEnable = properties.enableDepthTest ? TRUE : FALSE,
			.DepthWriteMask = properties.enableDepthTest ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO,
			.DepthFunc = D3D12_COMPARISON_FUNC_LESS,
			.StencilEnable = FALSE,
			.StencilReadMask = 0,
			.StencilWriteMask = 0,
			.FrontFace = stencilOp,
			.BackFace = stencilOp,
		};

		//RTVs, from the render target reflection of the pixel shader
		D3D12_RT_FORMAT_ARRAY renderTargetFormats = rasterizationProgram->GetRenderTargetFormats();
		pipelineStateDesc.NumRenderTargets = renderTargetFormats.NumRenderTargets;

		for (UINT i = 0; i < renderTargetFormats.NumRenderTargets; i++)
			pipelineStateDesc.RTVFormats[i] = renderTargetFormats.RTFormats[i];

		//DSV
		pipelineStateDesc.DSVFormat = properties.enableDepthTest ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_UNKNOWN;

		//Sampling
		pipelineStateDesc.SampleDesc = DXGI_SAMPLE_DESC{
			.Count = 1,
			.Quality = 0,
		};

		pipelineStateDesc.NodeMask = 0;
		pipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		ComPtr<ID3D12PipelineState> pipelineState;

		if (FAILED(m_device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pipelineState)))) {
			error = "Failed to create rasterization pipeline state";
			return nullptr;
		}

		return std::make_shared<Rasterization::DX12RasterizationPipelineModule>(m_device, pipelineState, rootSignature, material, properties.topologyType);
	}
}
