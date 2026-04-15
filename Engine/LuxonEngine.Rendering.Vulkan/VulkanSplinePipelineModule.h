#pragma once
#include "Core/Vector2.h"
#include "Core/Vector3.h"
#include "vulkan-pch.h"

namespace QuantumEngine::Rendering {
	class SplineRenderer;
}

namespace QuantumEngine::Rendering::Vulkan {

	namespace Compute {
		class SPIRVComputeProgram;
	}

	namespace Rasterization {
		class VulkanRasterizationMaterial;
		class SPIRVRasterizationProgram;
	}

	struct SplineEntityData {
		ref<SplineRenderer> splineRenderer;
		ref<Rasterization::VulkanRasterizationMaterial> material;
		ref<Compute::SPIRVComputeProgram> computeProgram;
	};

	struct SplineVertex {
		Vector3 position;
		Vector2 uv;
		Vector3 normal;
		Vector3 tangent;

		SplineVertex() : SplineVertex(Vector3(0.0f), Vector2(0.0f), Vector3(0.0f), Vector3(0.0f))
		{
		}

		SplineVertex(const Vector3& pos, const Vector2& tex, const Vector3& normal, const Vector3& tangent)
			:position(pos), uv(tex), normal(normal), tangent(tangent)
		{
		}
	};

	struct SplineParameters {
		Vector3 startPoint;
		float width;
		Vector3 midPoint;
		float length;
		Vector3 endPoint;
	};

	class VulkanSplinePipelineModule {
	public:
		VulkanSplinePipelineModule(const VkDevice device);
		~VulkanSplinePipelineModule();
		bool Initialize(const SplineEntityData& splineEntity, const VkRenderPass renderPass, const VkDescriptorPool pool);
		void ComputeCommand(VkCommandBuffer commandBuffer);
		void RenderCommand(VkCommandBuffer commandBuffer);
		void WriteOffset(const std::string name, UInt32 offsetIndex);
	private:
		bool InitializeVertexBuffer(const SplineEntityData& splineEntity);
		bool InitializeComputePipeline(const SplineEntityData& splineEntity, const VkDescriptorPool pool);
		bool InitializeGraphicsPipeline(const SplineEntityData& splineEntity, const VkRenderPass renderPass);
		
		static VkVertexInputBindingDescription s_bindingDescriptions;
		static VkVertexInputAttributeDescription s_attributeDescriptions[4];
		static VkPipelineVertexInputStateCreateInfo s_vertexInputInfo;
		
		
		VkDevice m_device;
		VkBuffer m_vertexBuffer;
		VkDeviceMemory m_vertexBufferMemory;

		ref<Compute::SPIRVComputeProgram> m_computeProgram;
		VkPipeline m_computePipeline;
		VkDescriptorSet m_computeDescriptorSet;
		SplineParameters m_splineParameters;
		UInt32 m_widthOffset;
		ref<SplineRenderer> m_splineRenderer;

		VkPipeline m_graphicsPipeline;
		ref<Rasterization::VulkanRasterizationMaterial> m_material;
		ref<Rasterization::SPIRVRasterizationProgram> m_program;
		std::vector<UInt32> m_offset;
	};
}