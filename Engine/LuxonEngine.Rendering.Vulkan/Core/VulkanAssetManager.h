#pragma once
#include "vulkan-pch.h"
#include "Rendering/GPUAssetManager.h"
#include <map>

namespace QuantumEngine::Rendering::Vulkan {
	class VulkanTexture2DController;
	class VulkanMeshController;
	class VulkanBufferFactory;

	class VulkanAssetManager : public GPUAssetManager 
	{
	public:
		VulkanAssetManager(const VkDevice device, VkPhysicalDevice physicalDevice);
		~VulkanAssetManager();
		bool Initializes(UInt32 familyIndex);
		virtual void UploadMeshToGPU(const ref<Mesh>& mesh) override;
		virtual void UploadTextureToGPU(const ref<Texture2D>& texture) override;
		virtual void UploadMeshesToGPU(const std::vector<ref<Mesh>>& meshes) override;
		virtual void UnloadAssets() override;

	private:
		VkDevice m_device;
		VkPhysicalDevice m_physicalDevice;
		ref<VulkanBufferFactory> m_bufferFactory;
		VkCommandPool m_commandPool;
		VkCommandBuffer m_commandBuffer;
		VkQueue m_graphicsQueue;
		VkPhysicalDeviceMemoryProperties m_memoryProperties;

		std::map<ref<Texture2D>, ref<VulkanTexture2DController>> m_texturePairs;
		std::map<ref<Mesh>, ref<VulkanMeshController>> m_meshPairs;
	};
}