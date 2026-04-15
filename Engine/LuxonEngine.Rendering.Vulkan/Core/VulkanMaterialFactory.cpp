#include "vulkan-pch.h"
#include "VulkanMaterialFactory.h"
#include "Core/SPIRVShaderProgram.h"
#include "Rasterization/SPIRVRasterizationProgram.h"
#include "RayTracing/SPIRVRayTracingProgram.h"
#include "Rendering/Material.h"


ref<QuantumEngine::Rendering::Material> QuantumEngine::Rendering::Vulkan::VulkanMaterialFactory::CreateMaterial(const ref<ShaderProgram>& program)
{
	ref<Rasterization::SPIRVRasterizationProgram> rasterProgram = std::dynamic_pointer_cast<Rasterization::SPIRVRasterizationProgram>(program);
	
	if (rasterProgram != nullptr) {
		auto matReflection = rasterProgram->GetReflection().CreateMaterialReflection();
		return std::make_shared<Material>(program, &matReflection);
	}

	ref<RayTracing::SPIRVRayTracingProgram> rayTracingProgram = std::dynamic_pointer_cast<RayTracing::SPIRVRayTracingProgram>(program);
	
	if (rayTracingProgram != nullptr) {
		auto matReflection = rayTracingProgram->GetReflection().CreateMaterialReflection();
		auto& shaderRecords = rayTracingProgram->GetShaderRecordVariables();

		UInt32 fieldIndex = 0;

		for (auto& shaderRecordVar : shaderRecords) {
			if (shaderRecordVar.name[0] == '_') {// Skip internal variables
				continue;
			}

			MaterialValueFieldInfo valueFieldInfo{
				.name = shaderRecordVar.name,
				.fieldIndex = fieldIndex,
				.size = shaderRecordVar.size,
			};

			matReflection.valueFields.push_back(valueFieldInfo);
			fieldIndex++;
		}

		UInt32 textureIndex = 0;

		for (auto& texFiels : matReflection.textureFields) {
			texFiels.fieldIndex = textureIndex;
			textureIndex++;
		}

		return std::make_shared<Material>(program, &matReflection);
	}
	
	return std::make_shared<Material>(program);
}
