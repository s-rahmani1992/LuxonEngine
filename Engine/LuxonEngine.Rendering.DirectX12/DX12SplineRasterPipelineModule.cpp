#include "pch.h"
#include "DX12SplineRasterPipelineModule.h"
#include "DX12utilities.h"
#include "Rendering/SplineRenderer.h"
#include <Rendering/Material.h>
#include "Rasterization/DX12RasterizationMaterial.h"
#include "HLSLShader.h"
#include "Rasterization/HLSLRasterizationProgram.h"
#include "DX12HybridContext.h"
#include "Compute/HLSLComputeProgram.h"

#define SPLINE_WIDTH_FIELD_NAME "_width"
#define SPLINE_WIDTH_BUFFER_NAME "_CurveProperties"

D3D12_INPUT_ELEMENT_DESC QuantumEngine::Rendering::DX12::DX12SplineRasterPipelineModule::s_inputElementDescs[4] = {
	{ "position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

D3D12_INPUT_LAYOUT_DESC QuantumEngine::Rendering::DX12::DX12SplineRasterPipelineModule::s_layoutDesc = {
	.pInputElementDescs = s_inputElementDescs,
	.NumElements = 4,
};

QuantumEngine::Rendering::DX12::DX12SplineRasterPipelineModule::DX12SplineRasterPipelineModule(const SplineRendererData& splineData, DXGI_FORMAT depthFormat, const ref<Compute::HLSLComputeProgram>& computeProgram)
	:m_splineRenderer(splineData.renderer), m_transformHeapHandle(splineData.transformHandle), 
	m_material(splineData.material), m_depthFormat(depthFormat), m_splineWidth(splineData.renderer->GetWidth()),
	m_computeProgram(computeProgram)
{
	auto program = m_material->GetProgram();
	auto reflection = program->GetReflectionData();
	auto& rootConstantBuffer = reflection->GetRootConstants();
	/*auto widthIt = std::find_if(
		rootConstantList.begin(),
		rootConstantList.end(),
		[](const RootConstantBufferData& binding) {
			return binding.name == SPLINE_WIDTH_BUFFER_NAME;
		});*/

	m_widthRootIndex = rootConstantBuffer.rootParameterIndex;

	auto computeReflection = computeProgram->GetReflectionData();
	auto& computeRootConstantList = computeReflection->GetRootConstants();

	/*auto curveIt = std::find_if(
		computeRootConstantList.begin(),
		computeRootConstantList.end(),
		[](const RootConstantBufferData& binding) {
			return binding.name == SPLINE_WIDTH_BUFFER_NAME;
		});*/

	m_curveRootIndex = rootConstantBuffer.rootParameterIndex;

	auto& resourceVariableList = computeReflection->GetResourceVariables();

	auto vertexIt = std::find_if(
		resourceVariableList.begin(),
		resourceVariableList.end(),
		[](const ResourceVariableData& binding) {
			return binding.name == "_vertexBuffer";
		});

	m_vertexRootIndex = (*vertexIt).rootParameterIndex;

	m_splineParams.startPoint = m_splineRenderer->GetCurve().m_point1;
	m_splineParams.midPoint = m_splineRenderer->GetCurve().m_point2;
	m_splineParams.endPoint = m_splineRenderer->GetCurve().m_point3;
	m_splineParams.width = m_splineWidth;
	m_splineParams.length = m_splineRenderer->GetCurve().InterpolateLength(1.0f);
}

bool QuantumEngine::Rendering::DX12::DX12SplineRasterPipelineModule::Initialize(const ComPtr<ID3D12Device10>& device)
{
	m_device = device;
	//Create vertex buffer

	D3D12_RESOURCE_DESC vbDesc = ResourceUtilities::GetCommonBufferResourceDesc(
		sizeof(SplineVertex) * (m_splineRenderer->GetSegments() + 1), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	if (FAILED(device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &vbDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBuffer))))
		return false;

	m_vertexBuffer->SetName(L"Spline Vertex Buffer");

	// Buffer views
	m_bufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_bufferView.SizeInBytes = sizeof(SplineVertex) * (m_splineRenderer->GetSegments() + 1);
	m_bufferView.StrideInBytes = sizeof(SplineVertex);

	// Compute Pipeline to generate spline vertices
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineDesc = {};
	computePipelineDesc.pRootSignature = m_computeProgram->GetRootSignature().Get();
	computePipelineDesc.CS.BytecodeLength = m_computeProgram->GetCodeLength();
	computePipelineDesc.CS.pShaderBytecode = m_computeProgram->GetByteCode();
	computePipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	if (FAILED(device->CreateComputePipelineState(&computePipelineDesc, IID_PPV_ARGS(&m_computePipeline))))
		return false;

	// Create Pipeline State Object
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc;

	//Input part
	pipelineStateDesc.InputLayout = s_layoutDesc;
	pipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

	//Shader Part

	auto program = std::dynamic_pointer_cast<QuantumEngine::Rendering::DX12::Rasterization::HLSLRasterizationProgram>(m_material->GetMaterial()->GetProgram());
	m_rootSignature = program->GetRootSignature();
	auto vertexShader = program->GetVertexShader();
	auto pixelShader = program->GetPixelShader();
	auto geometryShader = program->GetGeometryShader();
	pipelineStateDesc.VS.BytecodeLength = vertexShader->GetCodeSize();
	pipelineStateDesc.VS.pShaderBytecode = vertexShader->GetByteCode();
	pipelineStateDesc.PS.BytecodeLength = pixelShader->GetCodeSize();
	pipelineStateDesc.PS.pShaderBytecode = pixelShader->GetByteCode();
	pipelineStateDesc.DS.pShaderBytecode = nullptr;
	pipelineStateDesc.DS.BytecodeLength = 0;
	pipelineStateDesc.HS.pShaderBytecode = nullptr;
	pipelineStateDesc.HS.BytecodeLength = 0;
	pipelineStateDesc.GS.pShaderBytecode = geometryShader->GetByteCode();
	pipelineStateDesc.GS.BytecodeLength = geometryShader->GetCodeSize();

	//Root Signature
	pipelineStateDesc.pRootSignature = m_rootSignature.Get();

	//Rasterizer
	pipelineStateDesc.RasterizerState;
	pipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	pipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
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
	pipelineStateDesc.DSVFormat = m_depthFormat;

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

void QuantumEngine::Rendering::DX12::DX12SplineRasterPipelineModule::Render(ComPtr<ID3D12GraphicsCommandList7>& commandList, D3D12_GPU_DESCRIPTOR_HANDLE camHandle, D3D12_GPU_DESCRIPTOR_HANDLE lightHandle)
{
	if (m_splineRenderer->IsDirty()) {
		m_splineParams.startPoint = m_splineRenderer->GetCurve().m_point1;
		m_splineParams.midPoint = m_splineRenderer->GetCurve().m_point2;
		m_splineParams.endPoint = m_splineRenderer->GetCurve().m_point3;
		m_splineParams.width = m_splineWidth;
		m_splineParams.length = m_splineRenderer->GetCurve().InterpolateLength(1.0f);

		commandList->SetComputeRootSignature(m_computeProgram->GetRootSignature().Get());
		commandList->SetComputeRoot32BitConstants(m_curveRootIndex, 12, &m_splineParams, 0);
		commandList->SetComputeRootDescriptorTable(m_vertexRootIndex, m_vertexBufferHandle);
		commandList->SetPipelineState(m_computePipeline.Get());

		D3D12_RESOURCE_BARRIER uavVertexBarrier
		{
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
			{
			.pResource = m_vertexBuffer.Get(),
			.Subresource = 0,
			.StateBefore = D3D12_RESOURCE_STATE_COMMON,
			.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			},
		};
		commandList->ResourceBarrier(1, &uavVertexBarrier);

		commandList->Dispatch(m_splineRenderer->GetSegments() + 1, 1, 1);

		D3D12_RESOURCE_BARRIER uavVertexEndBarrier = uavVertexBarrier;
		uavVertexEndBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		uavVertexEndBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
		commandList->ResourceBarrier(1, &uavVertexEndBarrier);
	}
	
	//Pipeline
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	commandList->SetPipelineState(m_pipeline.Get());

	//Input Assembler
	commandList->IASetVertexBuffers(0, 1, &m_bufferView);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);

	//Set Material Variables
	m_material->BindParameters(commandList); 
	commandList->SetGraphicsRoot32BitConstants(m_widthRootIndex, 1, &m_splineWidth, 0);
	m_material->BindCameraDescriptor(commandList, camHandle);
	m_material->BindLightDescriptor(commandList, lightHandle);
	m_material->BindTransformDescriptor(commandList, m_transformHeapHandle);

	//Draw Call
	commandList->DrawInstanced(m_splineRenderer->GetSegments() + 1, 1, 0, 0);
}

UInt32 QuantumEngine::Rendering::DX12::DX12SplineRasterPipelineModule::BindDescriptorToResources(const ComPtr<ID3D12DescriptorHeap>& descriptorHeap, UInt32 offset)
{
	auto incrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += offset * incrementSize;
	gpuHandle.ptr += offset * incrementSize;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = m_splineRenderer->GetSegments() + 1;
	uavDesc.Buffer.StructureByteStride = sizeof(SplineVertex);
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	m_device->CreateUnorderedAccessView(m_vertexBuffer.Get(), nullptr, &uavDesc, cpuHandle);
	m_vertexBufferHandle = gpuHandle;

	return 1;
}
