#pragma once
#include "vulkan-pch.h"
#include "Rendering/MaterialFactory.h"

namespace LuxonEngine::Rendering::Vulkan {
	class VulkanMaterialFactory : public MaterialFactory
	{
		virtual ref<Material> CreateMaterial(const ref<ShaderProgram>& program) override;
	};
}