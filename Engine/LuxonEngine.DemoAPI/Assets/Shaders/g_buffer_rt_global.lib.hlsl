#include "Common/TransformStructs.hlsli"
#include "Common/RTStructs.hlsli"

CAMERA_VAR(b0)

CONSTANT_VARIABLES_BEGIN
    uint _missIndex;
CONSTANT_VARIABLES_END(constantVars, b1)

#define _missIndex constantVars._missIndex

RT_SCENE_VAR(t0)

RT_OUT_TEXTURE_VAR(u0)

TEXTURE(_MaskTexture, uint, t1)

TEXTURE(_PositionTexture, float4, t2)

TEXTURE(_NormalTexture, float4, t3)

SAMPLER(mainSampler, s0)

[shader("raygeneration")]
void rayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();
    float2 uv = float2(launchIndex.xy) / float2(launchDim.xy);
    uint mask = _MaskTexture.Load(int3(launchIndex.xy, 0));
    
    if (mask == 0)
    {
        _OutputTexture[launchIndex.xy] = float4(0, 0, 0, 0);
        return;
    }
    
    float3 pos = _PositionTexture.SampleLevel(mainSampler, uv, 0).xyz;
    float3 norm = 2 * _NormalTexture.SampleLevel(mainSampler, uv, 0).xyz - 1;
    
    RayDesc ray;
    ray.Origin = pos;
    ray.Direction = normalize(reflect(-cameraData.position + pos.xyz, norm));
    ray.TMin = 0.1;
    ray.TMax = 100000;

    GeneralPayload payLoad;
    payLoad.recursionCount = 1;
    payLoad.targetMode = 1;
    TraceRay(_RTScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, _missIndex, ray, payLoad);
    if(payLoad.hit == 0)
        _OutputTexture[launchIndex.xy] = float4(0.2f, 0.4f, 0.6f, 0.0f);
    else
        _OutputTexture[launchIndex.xy] = float4(payLoad.color, 1.0f);
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.color = float3(0.2f, 0.4f, 0.6f);
    payload.hit = 0;
}