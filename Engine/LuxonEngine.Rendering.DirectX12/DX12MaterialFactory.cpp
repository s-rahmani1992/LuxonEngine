#include "pch.h"
#include "DX12MaterialFactory.h"
#include "Rasterization/HLSLRasterizationProgram.h"
#include "RayTracing/HLSLRayTracingProgram.h"
#include "Rendering/Material.h"

namespace Render = QuantumEngine::Rendering;

ref<Render::Material> Render::DX12::DX12MaterialFactory::CreateMaterial(const ref<Render::ShaderProgram>& program)
{
	return BuildMaterial(program);
}

ref<Render::Material> QuantumEngine::Rendering::DX12::DX12MaterialFactory::BuildMaterial(const ref<ShaderProgram>& program)
{
	// check if program is HLSL Rasterization Program
	auto hlslProgram = std::dynamic_pointer_cast<Render::DX12::Rasterization::HLSLRasterizationProgram>(program);

	if (hlslProgram != nullptr) {
		MaterialReflection reflectionData = hlslProgram->GetReflectionData()->CreateMaterialReflection(false);

		return std::make_shared<Render::Material>(program, &reflectionData);
	}

	auto hlslRTProgram = std::dynamic_pointer_cast<Render::DX12::RayTracing::HLSLRayTracingProgram>(program);

	if (hlslRTProgram != nullptr) {
		MaterialReflection reflectionData = hlslRTProgram->GetReflectionData()->CreateMaterialReflection(true);

		return std::make_shared<Render::Material>(program, &reflectionData);
	}

	return nullptr;
}
