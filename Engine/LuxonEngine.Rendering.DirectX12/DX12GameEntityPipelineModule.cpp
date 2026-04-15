#include "pch.h"
#include "DX12GameEntityPipelineModule.h"
#include "Core/GameEntity.h"
#include "Core/Mesh.h"
#include "DX12MeshController.h"
#include "DX12HybridContext.h"
#include "HLSLShader.h"
#include "DX12Utilities.h"
#include "Rendering/MeshRenderer.h"
#include "Rasterization/HLSLRasterizationProgram.h"
#include "Rasterization/DX12RasterizationMaterial.h"

bool QuantumEngine::Rendering::DX12::DX12GameEntityPipelineModule::Initialize(const ComPtr<ID3D12Device10>& device, const DX12MeshRendererGPUData& meshRendererData, DXGI_FORMAT depthFormat)
{
	m_transformHeapHandle = meshRendererData.transformHandle;

	// Create Pipeline State Object
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc;

	//Input part
	m_meshController = std::dynamic_pointer_cast<DX12MeshController>(meshRendererData.meshRenderer->GetMesh()->GetGPUHandle());
	pipelineStateDesc.InputLayout = *(DX12MeshController::GetLayoutDesc());
	pipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//Shader Part
	m_material = meshRendererData.material;
	auto program = std::dynamic_pointer_cast<QuantumEngine::Rendering::DX12::Rasterization::HLSLRasterizationProgram>(m_material->GetMaterial()->GetProgram());
	m_rootSignature = program->GetRootSignature();
	auto vertexShader = program->GetVertexShader();
	auto pixelShader = program->GetPixelShader();
	pipelineStateDesc.VS.BytecodeLength = vertexShader->GetCodeSize();
	pipelineStateDesc.VS.pShaderBytecode = vertexShader->GetByteCode();
	pipelineStateDesc.PS.BytecodeLength = pixelShader->GetCodeSize();
	pipelineStateDesc.PS.pShaderBytecode = pixelShader->GetByteCode();
	pipelineStateDesc.DS.pShaderBytecode = nullptr;
	pipelineStateDesc.DS.BytecodeLength = 0;
	pipelineStateDesc.HS.pShaderBytecode = nullptr;
	pipelineStateDesc.HS.BytecodeLength = 0;
	pipelineStateDesc.GS.pShaderBytecode = nullptr;
	pipelineStateDesc.GS.BytecodeLength = 0;

	//Root Signature
	pipelineStateDesc.pRootSignature = m_rootSignature.Get();

	//Rasterizer
	pipelineStateDesc.RasterizerState;
	pipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	pipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	pipelineStateDesc.RasterizerState.FrontCounterClockwise = FALSE;
	pipelineStateDesc.RasterizerState.DepthBias = 0;
	pipelineStateDesc.RasterizerState.DepthBiasClamp = 0.0f;
	pipelineStateDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
	pipelineStateDesc.RasterizerState.DepthClipEnable = FALSE;
	pipelineStateDesc.RasterizerState.MultisampleEnable = FALSE;
	pipelineStateDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	pipelineStateDesc.RasterizerState.ForcedSampleCount = 0;
	pipelineStateDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	//Stream Output
	pipelineStateDesc.StreamOutput.NumEntries = 0;
	pipelineStateDesc.StreamOutput.NumStrides = 0;
	pipelineStateDesc.StreamOutput.pBufferStrides = nullptr;
	pipelineStateDesc.StreamOutput.pSODeclaration = nullptr;
	pipelineStateDesc.StreamOutput.RasterizedStream = 0;

	//Flags and Cache
	pipelineStateDesc.CachedPSO.CachedBlobSizeInBytes = 0;
	pipelineStateDesc.CachedPSO.pCachedBlob = nullptr;
	pipelineStateDesc.NodeMask = 0;
	pipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	//RTVs
	pipelineStateDesc.NumRenderTargets = 1;
	pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineStateDesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;

	//DSV
	pipelineStateDesc.DSVFormat = depthFormat;

	//Blend
	pipelineStateDesc.BlendState.AlphaToCoverageEnable = FALSE;
	pipelineStateDesc.BlendState.IndependentBlendEnable = FALSE;

	//Blending for Color RTV
	pipelineStateDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
	pipelineStateDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
	pipelineStateDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
	pipelineStateDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	pipelineStateDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	pipelineStateDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
	pipelineStateDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	pipelineStateDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	pipelineStateDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	pipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//Depth buffer
	pipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
	pipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	pipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	pipelineStateDesc.DepthStencilState.StencilEnable = FALSE;
	pipelineStateDesc.DepthStencilState.StencilReadMask = 0;
	pipelineStateDesc.DepthStencilState.StencilWriteMask = 0;
	pipelineStateDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pipelineStateDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	pipelineStateDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	//Sampling
	pipelineStateDesc.SampleMask = 0xFFFFFFFF;
	pipelineStateDesc.SampleDesc.Count = 1;
	pipelineStateDesc.SampleDesc.Quality = 0;

	//Creating pipeline
	if (FAILED(device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&m_pipeline))))
		return false;

	return true;
}

void QuantumEngine::Rendering::DX12::DX12GameEntityPipelineModule::Render(ComPtr<ID3D12GraphicsCommandList7>& commandList, D3D12_GPU_DESCRIPTOR_HANDLE camHandle, D3D12_GPU_DESCRIPTOR_HANDLE lightHandle)
{
	//Pipeline
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	commandList->SetPipelineState(m_pipeline.Get());

	//Input Assembler
	commandList->IASetVertexBuffers(0, 1, m_meshController->GetVertexView());
	commandList->IASetIndexBuffer(m_meshController->GetIndexView());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//Set Material Variables
	m_material->BindParameters(commandList);
	m_material->BindCameraDescriptor(commandList, camHandle);
	m_material->BindLightDescriptor(commandList, lightHandle);
	m_material->BindTransformDescriptor(commandList, m_transformHeapHandle);

	//Draw Call
	commandList->DrawIndexedInstanced(m_meshController->GetMesh()->GetIndexCount(), 1, 0, 0, 0);
}
