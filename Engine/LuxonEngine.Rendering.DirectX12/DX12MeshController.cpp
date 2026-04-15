#include "pch.h"
#include "DX12MeshController.h"
#include "Core/Mesh.h"
#include "DX12Utilities.h"

D3D12_INPUT_ELEMENT_DESC QuantumEngine::Rendering::DX12::DX12MeshController::s_inputElementDescs[3] = {
	{ "position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

D3D12_INPUT_LAYOUT_DESC QuantumEngine::Rendering::DX12::DX12MeshController::s_layoutDesc = {
	.pInputElementDescs = s_inputElementDescs,
	.NumElements = 3,
};

QuantumEngine::Rendering::DX12::DX12MeshController::DX12MeshController(const ref<Mesh>& mesh)
	:m_mesh(mesh)
{
}

D3D12_SHADER_RESOURCE_VIEW_DESC QuantumEngine::Rendering::DX12::DX12MeshController::GetVertexSRVDesc()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC vertexView{
	.Format = DXGI_FORMAT_UNKNOWN,
	.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
	.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
	.Buffer = D3D12_BUFFER_SRV{
		.FirstElement = 0,
		.NumElements = m_mesh->GetVertexCount(),
		.StructureByteStride = sizeof(Vertex),
		.Flags = D3D12_BUFFER_SRV_FLAG_NONE,
		},
	};

	return vertexView;
}

D3D12_SHADER_RESOURCE_VIEW_DESC QuantumEngine::Rendering::DX12::DX12MeshController::GetIndexSRVDesc()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC indexView{
	.Format = DXGI_FORMAT_UNKNOWN,
	.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
	.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
	.Buffer = D3D12_BUFFER_SRV{
		.FirstElement = 0,
		.NumElements = m_mesh->GetIndexCount(),
		.StructureByteStride = sizeof(UInt32),
		.Flags = D3D12_BUFFER_SRV_FLAG_NONE,
		},
	};

	return indexView;
}

bool QuantumEngine::Rendering::DX12::DX12MeshController::Initialize(const ComPtr<ID3D12Device10>& device)
{
	//Create upload and vertex buffers
	D3D12_HEAP_PROPERTIES uploadHeapProps
	{
	.Type = D3D12_HEAP_TYPE_UPLOAD,
	.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
	.CreationNodeMask = 0,
	.VisibleNodeMask = 0,
	};

	D3D12_RESOURCE_DESC bufferDesc = ResourceUtilities::GetCommonBufferResourceDesc(
		sizeof(Vertex) * m_mesh->GetVertexCount(), D3D12_RESOURCE_FLAG_NONE);
	
	D3D12_HEAP_PROPERTIES bufferHeapProps{};
	bufferHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	bufferHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	bufferHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	bufferHeapProps.CreationNodeMask = 0;
	bufferHeapProps.VisibleNodeMask = 0;

	if(FAILED(device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBuffer))))
		return false;

	//Create upload and index buffers
	
	D3D12_RESOURCE_DESC indexDesc = ResourceUtilities::GetCommonBufferResourceDesc(
		sizeof(UInt32) * m_mesh->GetIndexCount(), D3D12_RESOURCE_FLAG_NONE);
	
	if (FAILED(device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &indexDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_indexBuffer))))
		return false;

	// Buffer views
	m_bufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_bufferView.SizeInBytes = sizeof(Vertex) * m_mesh->GetVertexCount();
	m_bufferView.StrideInBytes = sizeof(Vertex);

	// Index view
	m_indexView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexView.SizeInBytes = sizeof(UInt32) * m_mesh->GetIndexCount();
	m_indexView.Format = DXGI_FORMAT_R32_UINT;

	D3D12_DESCRIPTOR_HEAP_DESC meshHeapDesc{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		.NumDescriptors = 1,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	};

	if (FAILED(device->CreateDescriptorHeap(&meshHeapDesc, IID_PPV_ARGS(&m_vertexHeap)))) {
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC vertexView{
	.Format = DXGI_FORMAT_UNKNOWN,
	.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
	.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
	.Buffer = D3D12_BUFFER_SRV{
		.FirstElement = 0,
		.NumElements = m_mesh->GetVertexCount(),
		.StructureByteStride = sizeof(Vertex),
		.Flags = D3D12_BUFFER_SRV_FLAG_NONE,
		},
	};

	device->CreateShaderResourceView(m_vertexBuffer, &vertexView, m_vertexHeap->GetCPUDescriptorHandleForHeapStart());

	if (FAILED(device->CreateDescriptorHeap(&meshHeapDesc, IID_PPV_ARGS(&m_indexHeap)))) {
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC indexView{
	.Format = DXGI_FORMAT_UNKNOWN,
	.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
	.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
	.Buffer = D3D12_BUFFER_SRV{
		.FirstElement = 0,
		.NumElements = m_mesh->GetIndexCount(),
		.StructureByteStride = sizeof(UInt32),
		.Flags = D3D12_BUFFER_SRV_FLAG_NONE,
		},
	};

	device->CreateShaderResourceView(m_indexBuffer, &indexView, m_indexHeap->GetCPUDescriptorHandleForHeapStart());
	
	return true;
}

void QuantumEngine::Rendering::DX12::DX12MeshController::CopyToGPU(const ComPtr<ID3D12Resource2>& uploadBuffer, ComPtr<ID3D12GraphicsCommandList7>& uploadCommandList, UInt32 offset, Byte* mapData)
{
	UInt32 vertexSize = sizeof(Vertex) * m_mesh->GetVertexCount();
	UInt32 indexSize = sizeof(UInt32) * m_mesh->GetIndexCount();
	
	m_mesh->CopyVertexData(mapData);
	uploadCommandList->CopyBufferRegion(m_vertexBuffer, 0, uploadBuffer.Get(), offset, vertexSize);
	m_mesh->CopyIndexData(mapData + vertexSize);
	uploadCommandList->CopyBufferRegion(m_indexBuffer, 0, uploadBuffer.Get(), offset + vertexSize, indexSize);
}

ComPtr<ID3D12Resource2> QuantumEngine::Rendering::DX12::DX12MeshController::CreateBLASResource(const ComPtr<ID3D12GraphicsCommandList7>& commandList, ComPtr<ID3D12Resource2>& scratchBuffer)
{
	ComPtr<ID3D12Device10> device;

	if (FAILED(commandList->GetDevice(IID_PPV_ARGS(&device))))
		return nullptr;

	D3D12_RAYTRACING_GEOMETRY_DESC rtMeshDesc = GetRTGeometryDesc();

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS rtInput{
		.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
		.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE,
		.NumDescs = 1,
		.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
		.pGeometryDescs = &rtMeshDesc,
	};

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO rtPreInfo;
	device->GetRaytracingAccelerationStructurePrebuildInfo(&rtInput, &rtPreInfo);

	D3D12_RESOURCE_DESC rtBLASDesc = ResourceUtilities::GetCommonBufferResourceDesc(rtPreInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	D3D12_RESOURCE_DESC rtScratchDesc = ResourceUtilities::GetCommonBufferResourceDesc(rtPreInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	ComPtr<ID3D12Resource2> BLASBuffer;

	if (FAILED(device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &rtBLASDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&BLASBuffer))))
		return nullptr;

	if (FAILED(device->CreateCommittedResource(&DescriptorUtilities::CommonDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &rtScratchDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&scratchBuffer))))
		return nullptr;


	D3D12_RESOURCE_BARRIER b1;
	b1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b1.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
	b1.Transition.pResource = scratchBuffer.Get();
	b1.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	b1.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	b1.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &b1);
	
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = rtInput;
	asDesc.DestAccelerationStructureData = BLASBuffer->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();

	commandList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = BLASBuffer.Get();
	commandList->ResourceBarrier(1, &uavBarrier);

	return BLASBuffer;
}

void QuantumEngine::Rendering::DX12::DX12MeshController::Release() {
	m_vertexHeap->Release();
	m_indexHeap->Release();
	m_vertexBuffer->Release();
	m_indexBuffer->Release();
}

D3D12_RAYTRACING_GEOMETRY_DESC QuantumEngine::Rendering::DX12::DX12MeshController::GetRTGeometryDesc() const
{
	return D3D12_RAYTRACING_GEOMETRY_DESC {
		.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
		.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE,
		.Triangles = D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC{
			.Transform3x4 = 0,
			.IndexFormat = DXGI_FORMAT_R32_UINT,
			.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT,
			.IndexCount = m_mesh->GetIndexCount(),
			.VertexCount = m_mesh->GetVertexCount(),
			.IndexBuffer = m_indexBuffer->GetGPUVirtualAddress(),
			.VertexBuffer = D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE{
				.StartAddress = m_vertexBuffer->GetGPUVirtualAddress(),
				.StrideInBytes = sizeof(Vertex),
			}
		}
	};
}
