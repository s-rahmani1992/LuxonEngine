#pragma once
#include "vulkan-pch.h"
#include "Rendering/GPUMeshController.h"

namespace QuantumEngine {
	class Mesh;
}

namespace QuantumEngine::Rendering::Vulkan {
	namespace RayTracing {
		struct VulkanBLASBuildInfo;
	}

	class VulkanBufferFactory;

	class VulkanMeshController : public GPUMeshController
	{
	public: 		
		VulkanMeshController(const ref<Mesh>& mesh, const VkDevice device);
		bool Initialize(const ref<VulkanBufferFactory>& bufferFactory);
		inline VkBuffer GetVertexBuffer() { return m_vertexBuffer; }
		inline VkBuffer GetIndexBuffer() { return m_indexBuffer; }
		void CopyCommand(VkCommandBuffer commandBuffer, VkBuffer stageBuffer, UInt32 offset);
		void GetBLASBuildInfo(RayTracing::VulkanBLASBuildInfo* blasBuildInfo);
		VkBuffer CreateVertexStorageBuffer();
		VkBuffer CreateIndexStorageBuffer();
		void Release();
	private:

		ref<Mesh> m_mesh;
		VkDevice m_device;

		VkBuffer m_vertexBuffer;
		VkDeviceMemory m_vertexBufferMemory;

		VkBuffer m_indexBuffer;
		VkDeviceMemory m_indexBufferMemory;
	};
}
