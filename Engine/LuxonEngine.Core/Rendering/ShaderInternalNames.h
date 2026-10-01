#pragma once
#include <string_view>
#include <unordered_set>

#define INTERNAL_CAMERA_DATA_NAME				"_CameraData"
#define INTERNAL_LIGHT_DATA_NAME				"_LightData"
#define INTERNAL_RT_TLAS_SCENE_NAME				"_RTScene"
#define INTERNAL_RT_OUTPUT_TEXTURE_NAME			"_OutputTexture"
#define INTERNAL_RT_MISS_INDEX_NAME				"_missIndex"
#define INTERNAL_RT_TRANSFORM_ARRAY_NAME		"_ObjectTransformDataArray"
#define INTERNAL_RT_VERTEX_BUFFER_ARRAY_NAME	"_vertexBufferArray"
#define INTERNAL_RT_INDEX_BUFFER_ARRAY_NAME		"_indexBufferArray"
#define INTERNAL_GBUFFER_POSITION_TEXTURE_NAME	"_PositionTexture"
#define INTERNAL_GBUFFER_NORMAL_TEXTURE_NAME	"_NormalTexture"
#define INTERNAL_GBUFFER_MASK_TEXTURE_NAME		"_MaskTexture"

#define INTERNAL_OBJECT_TRANSFORM_DATA_NAME		"_ObjectTransformData"
#define INTERNAL_VERTEX_BUFFER_NAME				"_vertexBuffer"
#define INTERNAL_INDEX_BUFFER_NAME				"_indexBuffer"
#define INTERNAL_MASK_TEXTURE_NAME				"_maskTexture"
#define INTERNAL_SPLINE_WIDTH_NAME				"_width"
#define INTERNAL_SPLINE_CURVE_PROPERTIES_NAME	"_CurveProperties"

namespace LuxonEngine::Rendering {
	class ShaderInternalNames {
	public:
		static bool IsInternal(std::string_view name) { return name.empty() == false && name[0] == '_'; }

		static bool IsGlobal(std::string_view name) { return GlobalNames().contains(name); }

		static bool IsPerEntity(std::string_view name) { return PerEntityNames().contains(name); }

	private:
		static const std::unordered_set<std::string_view>& GlobalNames() {
			static const std::unordered_set<std::string_view> names = {
				INTERNAL_CAMERA_DATA_NAME,
				INTERNAL_LIGHT_DATA_NAME,
				INTERNAL_RT_TLAS_SCENE_NAME,
				INTERNAL_RT_OUTPUT_TEXTURE_NAME,
				INTERNAL_RT_MISS_INDEX_NAME,
				INTERNAL_RT_TRANSFORM_ARRAY_NAME,
				INTERNAL_RT_VERTEX_BUFFER_ARRAY_NAME,
				INTERNAL_RT_INDEX_BUFFER_ARRAY_NAME,
				INTERNAL_GBUFFER_POSITION_TEXTURE_NAME,
				INTERNAL_GBUFFER_NORMAL_TEXTURE_NAME,
				INTERNAL_GBUFFER_MASK_TEXTURE_NAME,
			};
			return names;
		}

		static const std::unordered_set<std::string_view>& PerEntityNames() {
			static const std::unordered_set<std::string_view> names = {
				INTERNAL_OBJECT_TRANSFORM_DATA_NAME,
				INTERNAL_VERTEX_BUFFER_NAME,
				INTERNAL_INDEX_BUFFER_NAME,
				INTERNAL_MASK_TEXTURE_NAME,
				INTERNAL_SPLINE_WIDTH_NAME,
				INTERNAL_SPLINE_CURVE_PROPERTIES_NAME,
			};
			return names;
		}
	};
}
