#include "Common/TransformStructs.hlsli"
#include "Common/RTStructs.hlsli"

OBJECT_TRANSFORM_VAR(b0)

CONSTANT_VARIABLES_BEGIN
    float refractionFactor;
    uint maxRecursion;
    uint _missIndex;
CONSTANT_VARIABLES_END(constantVars, b1)

#define refractionFactor constantVars.refractionFactor
#define maxRecursion constantVars.maxRecursion
#define _missIndex constantVars._missIndex

RT_SCENE_VAR(t0)

RT_OBJECT_INDEX_BUFFER_VAR(t1)

RT_OBJECT_VERTEX_BUFFER_VAR(t2)

[shader("closesthit")]
void chs(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    if (payload.targetMode == 2)
    {
        payload.hit = 0;
        return;
    }
    
    float3 rayDirection = normalize(WorldRayDirection());
    float3 normal = CalculateNormal(_indexBuffer, _vertexBuffer, attribs.barycentrics);
    normal = normalize(mul(float4(normal, 1.0f), transformData.rotationMatrix).xyz);
    
    if (payload.recursionCount <= maxRecursion)
    {
        bool isReflect;
        RayDesc ray;
        ray.Origin = CalculeteHitPosition();
        ray.Direction = Refract(rayDirection, normal, refractionFactor, isReflect);
        ray.TMin = 0.1;
        ray.TMax = 100000;
        
        GeneralPayload innerPayload;
        innerPayload.targetMode = 1;
        innerPayload.recursionCount = payload.recursionCount + 1;
        TraceRay(_RTScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, ray, innerPayload);
        payload.color = innerPayload.color.xyz;
    }
    else
    {
        payload.color = float3(1.0f, 1.0f, 1.0f);
    }
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.color = float3(0.1f, 0.7f, 0.3f);
}