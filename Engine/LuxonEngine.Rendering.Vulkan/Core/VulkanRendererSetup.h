#pragma once
#include "vulkan-pch.h"
#include "BasicTypes.h"
#include <array>
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace LuxonEngine {
	class GameEntity;
}

namespace LuxonEngine::Rendering {
	class Renderer;
	class ShaderProgram;
}

namespace LuxonEngine::Rendering::Vulkan {
	class VulkanPipelineFactory;
	class VulkanBufferFactory;
	class VulkanAssetManager;
	class VulkanRenderPass;
	class VulkanComputePass;

	struct VulkanRendererEntity {
		GameEntity* entity = nullptr;
		Renderer* renderer = nullptr;
		VkBuffer transformBuffer = VK_NULL_HANDLE;
	};

	struct VulkanNamedDescriptor {
		const char* name = nullptr;
		VkImageView imageView = VK_NULL_HANDLE;
	};

	struct VulkanPassSetupContext {
		VkDevice device = VK_NULL_HANDLE;

		VulkanPipelineFactory* pipelineFactory = nullptr;
		VulkanBufferFactory* bufferFactory = nullptr;
		VulkanAssetManager* assetManager = nullptr;
		std::function<ShaderProgram*(const std::string&)> getInternalProgram;

		VkExtent2D renderExtent = {};
		VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
		std::array<float, 4> backgroundColor = {};

		const std::vector<ref<GameEntity>>* sceneEntities = nullptr;
		std::span<const VulkanRendererEntity> allEntities;

		VkBuffer cameraBuffer = VK_NULL_HANDLE;
		VkDeviceSize cameraStride = 0;
		VkBuffer lightBuffer = VK_NULL_HANDLE;
		VkDeviceSize lightStride = 0;

		std::vector<VkBuffer>* meshStorageBuffers = nullptr;
		std::vector<VulkanNamedDescriptor> globalDescriptors;

		VulkanComputePass* computePass = nullptr;
		VulkanRenderPass* mainRenderPass = nullptr;

		VulkanRenderPass* gBufferRenderPass = nullptr;
	};

	class IVulkanRendererSetup {
	public:
		virtual ~IVulkanRendererSetup() = default;

		virtual void Setup(std::span<VulkanRendererEntity> entities, VulkanPassSetupContext& context) = 0;

		virtual void Update() {}

		virtual void Teardown() {}
	};
}
