#include "pch.h"
#include "DX12GBufferPipelineModule.h"
#include "DX12Utilities.h"
#include "Core/Vector2UInt.h"
#include "Core/HLSLReflection.h"
#include "Rasterization/HLSLRasterizationProgram.h"
#include "DX12MeshController.h"
#include "Core/Mesh.h"
#include "Rendering/GBufferRTReflectionRenderer.h"
#include "DX12HybridContext.h"

bool QuantumEngine::Rendering::DX12::DX12GBufferPipelineModule::Initialize(const ComPtr<ID3D12Device10>& device, const Vector2UInt& size, const ref<Rasterization::HLSLRasterizationProgram>& gBufferProgram)
{
	m_program = gBufferProgram;
	auto r = m_program->GetReflectionData();
	m_cameraRootIndex = r->GetRootParameterIndexByName(HLSL_CAMERA_DATA_NAME);
	m_transformRootIndex = m_program->GetReflectionData()->GetRootParameterIndexByName(HLSL_OBJECT_TRANSFORM_DATA_NAME);
	m_rootSignature = m_program->GetRootSignature();
	m_size = size;

	/////// Create GBuffer Resources

	// Position Buffer
	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	bufferDesc.Format = m_gPositionFormat;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	bufferDesc.Width = size.x;
	bufferDesc.Height = size.y;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	bufferDesc.MipLevels = 1;
	bufferDesc.SampleDesc.Count = 1;

	D3D12_CLEAR_VALUE clearValue{
		.Format = m_gPositionFormat,
		.Color = { 0.0f, 0.0f, 0.0f, 0.0f }
	};

	if (FAILED(device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE,
		&bufferDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&m_gPositionBuffer)
	))) {
		return false;
	}

	// Normal Buffer
	bufferDesc.Format = m_gNormalFormat;
	clearValue.Format = m_gNormalFormat;

	if (FAILED(device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE,
		&bufferDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&m_gNormalBuffer)
	))) {
		return false;
	}

	// Mask Buffer
	bufferDesc.Format = m_gMaskFormat;
	clearValue.Format = m_gMaskFormat;

	if (FAILED(device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE,
		&bufferDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&m_gMaskBuffer)
	))) {
		return false;
	}

	/////// Create Render Target Descriptor and Views

	// Create Heap for Render Target view
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = 3,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0,
	};

	if (FAILED(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap))))
		return false;

	auto incSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto firstHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// Create RTV for Position Buffer
	D3D12_RENDER_TARGET_VIEW_DESC rtv_desc
	{
		.Format = m_gPositionFormat,
		.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
		.Texture2D = D3D12_TEX2D_RTV
		{
			.MipSlice = 0,
			.PlaneSlice = 0
		},
	};

	device->CreateRenderTargetView(m_gPositionBuffer.Get(), &rtv_desc, firstHandle);
	m_gPositionHandle = firstHandle;

	// Create RTV for Normal Buffer
	rtv_desc.Format = m_gNormalFormat;
	firstHandle.ptr += incSize;
	device->CreateRenderTargetView(m_gNormalBuffer.Get(), &rtv_desc, firstHandle);
	m_gNormalHandle = firstHandle;

	// Create RTV for Mask Buffer
	rtv_desc.Format = m_gMaskFormat;
	firstHandle.ptr += incSize;
	device->CreateRenderTargetView(m_gMaskBuffer.Get(), &rtv_desc, firstHandle);
	m_gMaskHandle = firstHandle;

	//////// Create SRV Descriptor Heap and Views
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		.NodeMask = 0,
	};

	if (FAILED(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_positionHeap))))
		return false;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{ 
		.Format = m_gPositionFormat,
		.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Texture2D = D3D12_TEX2D_SRV{.MipLevels = bufferDesc.MipLevels,},
	};

	device->CreateShaderResourceView(m_gPositionBuffer.Get(), &srvDesc, m_positionHeap->GetCPUDescriptorHandleForHeapStart());

	if(FAILED(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_normalHeap))))
		return false;

	srvDesc.Format = m_gNormalFormat;

	device->CreateShaderResourceView(m_gNormalBuffer.Get(), &srvDesc, m_normalHeap->GetCPUDescriptorHandleForHeapStart());

	if (FAILED(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_maskHeap))))
		return false;

	srvDesc.Format = m_gMaskFormat;
	device->CreateShaderResourceView(m_gMaskBuffer.Get(), &srvDesc, m_maskHeap->GetCPUDescriptorHandleForHeapStart());
	//////// Create Depth Buffer and View

	D3D12_RESOURCE_DESC depthResourceDesc;
	depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResourceDesc.Alignment = 0;
	depthResourceDesc.Width = size.x;
	depthResourceDesc.Height = size.y;
	depthResourceDesc.DepthOrArraySize = 1;
	depthResourceDesc.MipLevels = 1;
	depthResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthResourceDesc.SampleDesc.Count = 1;
	depthResourceDesc.SampleDesc.Quality = 0;
	depthResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.DepthStencil.Stencil = 0;

	if (FAILED(device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE,
		&depthResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue,
		IID_PPV_ARGS(&m_depthBuffer))))
		return false;

	D3D12_DESCRIPTOR_HEAP_DESC depthHeapDesc{
	.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
	.NumDescriptors = 1,
	.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	.NodeMask = 0,
	};

	if (FAILED(device->CreateDescriptorHeap(&depthHeapDesc, IID_PPV_ARGS(&m_depthHeap))))
		return false;

	D3D12_DEPTH_STENCIL_VIEW_DESC depthViewDesc{
		.Format = DXGI_FORMAT_D32_FLOAT,
		.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
		.Flags = D3D12_DSV_FLAG_NONE,
		.Texture2D = D3D12_TEX2D_DSV{.MipSlice = 0},
	};

	device->CreateDepthStencilView(
		m_depthBuffer.Get(), &depthViewDesc,
		m_depthHeap->GetCPUDescriptorHandleForHeapStart());

	//////// Create Pipeline State Object

	if (CreatePipelineState(device) == false)
		return false;

	return true;
}

void QuantumEngine::Rendering::DX12::DX12GBufferPipelineModule::RenderCommand(ComPtr<ID3D12GraphicsCommandList7>& commandList, D3D12_GPU_DESCRIPTOR_HANDLE camHandle)
{
	//Set Render Target
	D3D12_RESOURCE_BARRIER positionBeginBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_gPositionBuffer.Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_COMMON,
		.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
		},
	};
	commandList->ResourceBarrier(1, &positionBeginBarrier);

	D3D12_RESOURCE_BARRIER normalBeginBarrier = positionBeginBarrier;
	normalBeginBarrier.Transition.pResource = m_gNormalBuffer.Get();
	commandList->ResourceBarrier(1, &normalBeginBarrier);

	D3D12_RESOURCE_BARRIER maskBeginBarrier = positionBeginBarrier;
	maskBeginBarrier.Transition.pResource = m_gMaskBuffer.Get();
	commandList->ResourceBarrier(1, &maskBeginBarrier);

	commandList->ClearDepthStencilView(m_depthHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	float clearPositionAndNormal[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float clearMask = 0.0f;
	auto dsvHandle = m_depthHeap->GetCPUDescriptorHandleForHeapStart();
	commandList->ClearRenderTargetView(m_gPositionHandle, clearPositionAndNormal, 0, nullptr);
	commandList->ClearRenderTargetView(m_gNormalHandle, clearPositionAndNormal, 0, nullptr);
	commandList->ClearRenderTargetView(m_gMaskHandle, &clearMask, 0, nullptr);
	commandList->OMSetRenderTargets(3, &m_gPositionHandle, false, &dsvHandle);

	//Viewport
	D3D12_VIEWPORT viewPort{};
	viewPort.Height = m_size.y;
	viewPort.Width = m_size.x;
	viewPort.TopLeftX = viewPort.TopLeftY = 0;
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	commandList->RSSetViewports(1, &viewPort);

	//Rect Scissor
	RECT scissorRect{};
	scissorRect.left = scissorRect.top = 0;
	scissorRect.right = m_size.x;
	scissorRect.bottom = m_size.y;
	commandList->RSSetScissorRects(1, &scissorRect);

	//Pipeline
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	commandList->SetPipelineState(m_pipeline.Get());

	for(auto& entity : m_entities) {
		//Input Assembler
		commandList->IASetVertexBuffers(0, 1, entity.meshController->GetVertexView());
		commandList->IASetIndexBuffer(entity.meshController->GetIndexView());

		//Set Material Variables
		commandList->SetGraphicsRootDescriptorTable(m_cameraRootIndex, camHandle);
		commandList->SetGraphicsRootDescriptorTable(m_transformRootIndex, entity.transformHandle);

		//Draw Call
		commandList->DrawIndexedInstanced(entity.meshController->GetMesh()->GetIndexCount(), 1, 0, 0, 0);
	}
	
	D3D12_RESOURCE_BARRIER positionEndBarrier
	{
	.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
	.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	.Transition = D3D12_RESOURCE_TRANSITION_BARRIER
		{
		.pResource = m_gPositionBuffer.Get(),
		.Subresource = 0,
		.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
		.StateAfter = D3D12_RESOURCE_STATE_COMMON,
		},
	};
	commandList->ResourceBarrier(1, &positionEndBarrier);

	D3D12_RESOURCE_BARRIER normalEndBarrier = positionEndBarrier;
	normalEndBarrier.Transition.pResource = m_gNormalBuffer.Get();
	commandList->ResourceBarrier(1, &normalEndBarrier);

	D3D12_RESOURCE_BARRIER maskEndBarrier = positionEndBarrier;
	maskEndBarrier.Transition.pResource = m_gMaskBuffer.Get();
	commandList->ResourceBarrier(1, &maskEndBarrier);
}

void QuantumEngine::Rendering::DX12::DX12GBufferPipelineModule::PrepareEntities(const std::vector<EntityGBufferData>& entities)
{
	for(auto& entity : entities) {
		m_entities.push_back(BufferEntityData{
			.meshController = std::dynamic_pointer_cast<DX12MeshController>(entity.renderer->GetMesh()->GetGPUHandle()),
			.transformHandle = entity.transformHandle,
			});
	}
}

bool QuantumEngine::Rendering::DX12::DX12GBufferPipelineModule::CreatePipelineState(const ComPtr<ID3D12Device10>& device)
{
	/////// Create Pipeline State Object

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc;

	//Input part
	pipelineStateDesc.InputLayout = *(DX12MeshController::GetLayoutDesc());
	pipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//Shader Part
	//m_rootSignature = m_program->GetReflectionDatas()->rootSignature;
	auto vertexShader = m_program->GetVertexShader();
	auto pixelShader = m_program->GetPixelShader();
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
	pipelineStateDesc.NumRenderTargets = 3;
	pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	pipelineStateDesc.RTVFormats[1] = DXGI_FORMAT_R10G10B10A2_UNORM;
	pipelineStateDesc.RTVFormats[2] = DXGI_FORMAT_R8_UINT;
	pipelineStateDesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
	pipelineStateDesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;

	//DSV
	pipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

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
