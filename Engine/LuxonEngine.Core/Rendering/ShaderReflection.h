#pragma once
#include "../BasicTypes.h"
#include <string>
#include <vector>

namespace LuxonEngine::Rendering {
	enum class ShaderStageFlags : UInt32 {
		None			= 0,
		Vertex			= 1 << 0,
		Geometry		= 1 << 1,
		Pixel			= 1 << 2,
		Compute			= 1 << 3,
		RayGeneration	= 1 << 4,
		Miss			= 1 << 5,
		ClosestHit		= 1 << 6,
		AnyHit			= 1 << 7,
		Intersection	= 1 << 8,
		Callable		= 1 << 9,
		Amplification	= 1 << 10,
		Mesh			= 1 << 11,

		AllRasterization = Vertex | Geometry | Pixel,
		AllRayTracing = RayGeneration | Miss | ClosestHit | AnyHit | Intersection | Callable,
	};

	inline ShaderStageFlags operator|(ShaderStageFlags a, ShaderStageFlags b) { return ShaderStageFlags(UInt32(a) | UInt32(b)); }
	inline ShaderStageFlags operator&(ShaderStageFlags a, ShaderStageFlags b) { return ShaderStageFlags(UInt32(a) & UInt32(b)); }
	inline ShaderStageFlags& operator|=(ShaderStageFlags& a, ShaderStageFlags b) { return a = a | b; }
	inline bool HasStage(ShaderStageFlags flags, ShaderStageFlags stage) { return (flags & stage) != ShaderStageFlags::None; }

	enum class ShaderScalarType {
		Unknown,
		Bool,
		Int,
		UInt,
		Half,
		Float,
		Double,
	};

	enum class ShaderResourceKind {
		ConstantBuffer,
		StructuredBuffer,
		RWStructuredBuffer,
		Texture,
		RWTexture,	
		Sampler,
		AccelerationStructure,
	};

	struct ShaderConstantVariable {
		std::string name;
		UInt32 index = 0;
		UInt32 offset = 0;
		UInt32 size = 0;
		ShaderScalarType type = ShaderScalarType::Unknown;
		UInt32 rows = 1;
		UInt32 columns = 1;
	};

	struct ShaderConstantBlock {
		UInt32 offset = 0;
		UInt32 size = 0;
		bool isInternal = false;
		std::vector<ShaderConstantVariable> variables;
	};

	struct ShaderConstantData {
		std::string name;
		UInt32 size = 0;
		UInt32 binding = 0;
		UInt32 space = 0;
		ShaderStageFlags stages = ShaderStageFlags::None;
		std::vector<ShaderConstantBlock> blocks;
	};

	struct ShaderResourceVariable {
		std::string name;
		ShaderResourceKind kind = ShaderResourceKind::ConstantBuffer;
		UInt32 binding = 0;
		UInt32 space = 0;
		UInt32 count = 1;
		bool isUnbounded = false;
		bool isInternal = false;
		ShaderStageFlags stages = ShaderStageFlags::None;
	};

	struct ShaderVariableReflection {
		static constexpr UInt32 InvalidIndex = UINT32_MAX;

		static bool IsInternalName(const std::string& name) { return name.empty() == false && name[0] == '_'; }

		ShaderConstantData constants;
		std::vector<ShaderResourceVariable> resources;
		std::vector<ShaderResourceVariable> samplers;
	};
}
