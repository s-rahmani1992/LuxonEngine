#include "vulkan-pch.h"
#include "VulkanMaterialFactory.h"
#include "Core/SPIRVShaderProgram.h"
#include "Rasterization/SPIRVRasterizationProgram.h"
#include "RayTracing/SPIRVRayTracingProgram.h"
#include "Rendering/Material.h"


ref<LuxonEngine::Rendering::Material> LuxonEngine::Rendering::Vulkan::VulkanMaterialFactory::CreateMaterial(const ref<ShaderProgram>& program)
{
	return std::make_shared<Material>(program);
}
