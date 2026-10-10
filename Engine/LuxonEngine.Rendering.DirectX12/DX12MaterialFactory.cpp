#include "pch.h"
#include "DX12MaterialFactory.h"
#include "Rasterization/HLSLRasterizationProgram.h"
#include "RayTracing/HLSLRayTracingProgram.h"
#include "Rendering/Material.h"

namespace Render = LuxonEngine::Rendering;

ref<Render::Material> Render::DX12::DX12MaterialFactory::CreateMaterial(const ref<Render::ShaderProgram>& program)
{
	return BuildMaterial(program);
}

ref<Render::Material> LuxonEngine::Rendering::DX12::DX12MaterialFactory::BuildMaterial(const ref<ShaderProgram>& program)
{
	// programs using the shared reflection (e.g. mesh programs) create their fields from it
	return std::make_shared<Render::Material>(program);
}
