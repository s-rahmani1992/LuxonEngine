#include "Common/TransformStructs.hlsli"
#include "Common/RTStructs.hlsli"

CAMERA_VAR(b0)

CONSTANT_VARIABLES_BEGIN
    float4 missColor;
    float4 hitColor;
    uint _missIndex;
CONSTANT_VARIABLES_END(constantVars, b1)
    
#define missColor constantVars.missColor
#define hitColor constantVars.hitColor
#define _missIndex constantVars._missIndex

RT_SCENE_VAR(t0)

RT_OUT_TEXTURE_VAR(u0)

[shader("raygeneration")]
void rayGen()
{
    float3 pixelPos = CalculateScreenPosition(cameraData.inverseProjectionMatrix);
    
    RayDesc ray;
    ray.Origin = cameraData.position;
    ray.Direction = normalize(pixelPos - ray.Origin);
    ray.TMin = 0;
    ray.TMax = 100000;

    GeneralPayload payLoad;
    payLoad.recursionCount = 1;
    payLoad.targetMode = 0;
    TraceRay(_RTScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, _missIndex, 0, ray, payLoad);
    
    uint3 launchIndex = DispatchRaysIndex();
    _OutputTexture[launchIndex.xy] = float4(payLoad.color, 1);
}

[shader("closesthit")]
void chs(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.color = hitColor.xyz;
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.color = missColor.xyz;
}
