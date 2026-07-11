#include "VariableMacros.hlsli"

struct TransformData
{
    float4x4 modelMatrix;
    float4x4 rotationMatrix;
    float4x4 modelViewMatrix;
};

struct CameraData
{
    float4x4 projectionMatrix;
    float4x4 inverseProjectionMatrix;
    float4x4 viewMatrix;
    float3 position;
};


#if defined(_VULKAN)
    #ifdef _VK_RAY_TRACING
        #define OBJECT_TRANSFORM_VAR(x)    StructuredBuffer<TransformData> _ObjectTransformDataArray;
        #define transformData _ObjectTransformDataArray[InstanceIndex()]
    #else
#define OBJECT_TRANSFORM_VAR(x)    StructuredBuffer<TransformData> _ObjectTransformData;    \
        static const TransformData transformData = _ObjectTransformData[0];
    #endif
#else
    #define OBJECT_TRANSFORM_VAR(x)  cbuffer _ObjectTransformData : DX12_REGISTER_SPACE(x) {  \
        TransformData transformData; \
    };
#endif


#if defined(_VULKAN)
    #define CAMERA_VAR(x)  cbuffer _CameraData{ \
        CameraData cameraData; \
    };
#else
    #define CAMERA_VAR(x)  cbuffer _CameraData : DX12_REGISTER_SPACE(x) {  \
        CameraData cameraData; \
    };
#endif
