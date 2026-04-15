#include "pch.h"
#include "DX12AssetManager.h"
#include "DX12MeshController.h"
#include "DX12Texture2DController.h"
#include "DX12CommandExecuter.h"
#include "Core/Texture2D.h"
#include "Core/Mesh.h"
#include "DX12Utilities.h"

void QuantumEngine::Rendering::DX12::DX12AssetManager::UploadMeshToGPU(const ref<Mesh>& mesh)
{
	UploadMeshesToGPU({ mesh });
}

void QuantumEngine::Rendering::DX12::DX12AssetManager::UploadTextureToGPU(const ref<Texture2D>& texture)
{
	if (m_textures.find(texture) != m_textures.end()) { // mesh has already been uploaded
		return;
	}

	auto texture2DController = std::make_shared<DX12Texture2DController>(texture);

	if (texture2DController->Initialize(m_device) == false)
		return;

	ComPtr<ID3D12Resource2> uploadTextureBuffer;

	auto uploadBufferDesc = DX12::ResourceUtilities::GetCommonBufferResourceDesc(texture->GetTotalSize(), D3D12_RESOURCE_FLAG_NONE);

	if (FAILED(m_device->CreateCommittedResource(&DX12::DescriptorUtilities::CommonUploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&uploadTextureBuffer))))
		return;

	// Reset Commands
	m_uploadCommandAllocator->Reset();
	m_uploadCommandList->Reset(m_uploadCommandAllocator.Get(), nullptr);

	// Execute Upload Commands
	texture2DController->UploadToGPU(m_uploadCommandList, uploadTextureBuffer);
	m_meshUploadCommandExecuter->ExecuteAndWait(m_uploadCommandList.Get());
	texture->SetGPUHandle(texture2DController);
	m_textures.insert({ texture, texture2DController });
}

void QuantumEngine::Rendering::DX12::DX12AssetManager::UploadMeshesToGPU(const std::vector<ref<Mesh>>& meshes)
{
	UInt32 totalSize = 0;
	std::set<ref<Mesh>> meshesToUpload;

	for (auto& mesh : meshes) {
		if (mesh->IsUploadedToGPU()) // mesh has already been uploaded
			continue;

		auto it = meshesToUpload.emplace(mesh);

		if (it.second == false) // mesh is already listed for upload
			continue;

		totalSize += mesh->GetTotalSize();
	}

	auto uploadDesc = ResourceUtilities::GetCommonBufferResourceDesc(totalSize, D3D12_RESOURCE_FLAG_NONE);
	ComPtr<ID3D12Resource2> uploadBuffer;

	if (FAILED(m_device->CreateCommittedResource(&DescriptorUtilities::CommonUploadHeapProps,
		D3D12_HEAP_FLAG_NONE, &uploadDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer))))
	{
		return;
	}
	
	m_uploadCommandAllocator->Reset();
	m_uploadCommandList->Reset(m_uploadCommandAllocator.Get(), nullptr);

	UInt32 offset = 0;
	Byte* mappedData = nullptr;
	D3D12_RANGE range{ 0, totalSize - 1 };
	auto t = uploadBuffer->Map(0, &range, reinterpret_cast<void**>(&mappedData));

	for(auto& mesh : meshesToUpload) {
		auto meshController = std::make_shared<DX12MeshController>(mesh);

		if (meshController->Initialize(m_device) == false) { // if initialization fails, skip this mesh
			continue;
		}

		meshController->CopyToGPU(uploadBuffer, m_uploadCommandList, offset, mappedData);
		mesh->SetGPUHandle(meshController);
		m_uploadedMeshes.emplace(mesh, meshController);
		offset += mesh->GetTotalSize();
		mappedData += mesh->GetTotalSize();
	}
	uploadBuffer->Unmap(0, &range);
	m_meshUploadCommandExecuter->ExecuteAndWait(m_uploadCommandList.Get());
}

void QuantumEngine::Rendering::DX12::DX12AssetManager::UnloadAssets()
{
	for(auto& [texture, textureGPU] : m_textures) {
		texture->Release();
		textureGPU->Release();
	}

	for(auto& [mesh, meshGPU] : m_uploadedMeshes) {
		mesh->Release();
		meshGPU->Release();
	}
}

bool QuantumEngine::Rendering::DX12::DX12AssetManager::Initialize(ComPtr<ID3D12Device10>& device)
{
	m_device = device;

	//Command Queue
	ComPtr<ID3D12CommandQueue> cmdqueue;
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{
	.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
	.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
	.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
	.NodeMask = 0,
	};

	if (FAILED(device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdqueue))))
		return false;

	//fence
	ComPtr<ID3D12Fence1> fence;
	if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
		return false;

	m_meshUploadCommandExecuter = std::make_shared<DX12CommandExecuter>(cmdqueue, fence);
	
	if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_uploadCommandAllocator))))
		return false;

	if (FAILED(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_uploadCommandList))))
		return false;

	return true;
}
