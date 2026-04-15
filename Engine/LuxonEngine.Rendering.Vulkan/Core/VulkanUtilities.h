#pragma once
#include "vulkan-pch.h"

#define ALIGN(x,a) ((x + a - 1) / a) * a
#define HLSL_OBJECT_TRANSFORM_DATA_NAME "_ObjectTransformData"
#define HLSL_TRANSFORM_ARRAY "_ObjectTransformDataArray"
#define HLSL_CAMERA_DATA_NAME "_CameraData"
#define HLSL_LIGHT_DATA_NAME "_LightData"
#define HLSL_RT_TLAS_SCENE_NAME "_RTScene"
#define HLSL_RT_OUTPUT_TEXTURE_NAME "_OutputTexture"
#define HLSL_RT_VERTEX_BUFFER_ARRAY "_vertexBufferArray"
#define HLSL_RT_INDEX_BUFFER_ARRAY "_indexBufferArray"

UInt32 GetMemoryTypeIndex(const VkMemoryRequirements* memoryRequirement, VkMemoryPropertyFlags targetFlags, const VkPhysicalDeviceMemoryProperties* memoryProperties);

bool CreateBuffer(const VkPhysicalDevice physicalDevice, const VkDevice device, UInt32 size, UInt32 count, VkBuffer* buffer, VkDeviceMemory* memory);