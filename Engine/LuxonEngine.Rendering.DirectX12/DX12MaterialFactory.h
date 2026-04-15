#pragma once
#include "pch.h"
#include "Rendering/MaterialFactory.h"

namespace QuantumEngine::Rendering::DX12 {
	class DX12MaterialFactory : public MaterialFactory 
	{
		public:
			virtual ref<Material> CreateMaterial(const ref<ShaderProgram>& program) override;
			static ref<Material> BuildMaterial(const ref<ShaderProgram>& program);
	};
}