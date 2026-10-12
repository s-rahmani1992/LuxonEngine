#include "RendererRegistry.h"
#include "MeshRenderer.h"
#include "SplineRenderer.h"
#include "GBufferRTReflectionRenderer.h"
#include "SpikeMeshRenderer.h"
#include "SurfaceInstanceRenderer.h"
#include <iterator>

namespace LuxonEngine::Rendering {
	namespace {
		// the built-in renderers must have different ids
		constexpr RendererTypeId builtInIds[] = {
			MeshRenderer::StaticTypeId(), SplineRenderer::StaticTypeId(), GBufferRTReflectionRenderer::StaticTypeId(),
			SpikeMeshRenderer::StaticTypeId(), SurfaceInstanceRenderer::StaticTypeId(),
		};

		constexpr bool AllDistinct()
		{
			for (size_t i = 0; i < std::size(builtInIds); i++) {
				for (size_t j = i + 1; j < std::size(builtInIds); j++) {
					if (builtInIds[i] == builtInIds[j])
						return false;
				}
			}
			return true;
		}

		static_assert(AllDistinct(), "Two built-in renderer types have the same type id");
		static_assert(QualifiedTypeName<MeshRenderer>() == "LuxonEngine::Rendering::MeshRenderer", "The qualified type name cannot be read with this compiler");

		TypeRegistry& GetRegistry()
		{
			struct RendererTypes {
				TypeRegistry registry{ "renderer" };

				RendererTypes()
				{
					registry.Register<MeshRenderer>();
					registry.Register<SplineRenderer>();
					registry.Register<GBufferRTReflectionRenderer>();
					registry.Register<SpikeMeshRenderer>();
					registry.Register<SurfaceInstanceRenderer>();
				}
			};

			static RendererTypes types;
			return types.registry;
		}
	}

	bool RendererRegistry::Register(RendererTypeId id, const char* name)
	{
		return GetRegistry().Register(id, name);
	}

	const RendererTypeInfo* RendererRegistry::Find(RendererTypeId id)
	{
		return GetRegistry().Find(id);
	}
}
