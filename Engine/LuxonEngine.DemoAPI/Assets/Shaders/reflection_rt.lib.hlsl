#include "Common/TransformStructs.hlsli"
#include "Common/RTStructs.hlsli"
#include "Common/LightStructs.hlsli"

OBJECT_TRANSFORM_VAR(b0)

CAMERA_VAR(b1)

LIGHT_VAR(b2)

CONSTANT_VARIABLES_BEGIN
    uint castShadow;
    float reflectivity;
    float ambient;
    float diffuse;
    float specular;
CONSTANT_VARIABLES_END(constantVars, b3)

#define castShadow constantVars.castShadow
#define reflectivity constantVars.reflectivity
#define ambient constantVars.ambient
#define diffuse constantVars.diffuse
#define specular constantVars.specular

TEXTURE(mainTexture, float4, t0)

SAMPLER(mainSampler, s0);

RT_SCENE_VAR(t1)

RT_OBJECT_INDEX_BUFFER_VAR(t2)

RT_OBJECT_VERTEX_BUFFER_VAR(t3)

[shader("closesthit")]
void chs(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    if (payload.targetMode == 2 && castShadow > 0)
    {
        payload.hit = 1;
        return;
    }
    
    payload.hit = 1;
    
    float3 normal = CalculateNormal(_indexBuffer, _vertexBuffer, attribs.barycentrics);
    normal = mul(float4(normal, 1.0f), transformData.rotationMatrix).xyz;
    float2 uv = CalculateUV(_indexBuffer, _vertexBuffer, attribs.barycentrics);
    float3 position = CalculeteHitPosition();
    
    float4 texColor = mainTexture.SampleLevel(mainSampler, uv, 0);
    float3 ads = float3(ambient, diffuse, specular);
    float3 lightFactor = PhongLight(lightData, cameraData.position, position, normal, ads);
    
    if (payload.recursionCount <= 3)
    {
        float3 rayDirection = WorldRayDirection();
        
        RayDesc ray;
        ray.Origin = position;
        ray.Direction = normalize(reflect(rayDirection, normal));
        ray.TMin = 0.1;
        ray.TMax = 100000;
        
        GeneralPayload innerPayload;
        innerPayload.targetMode = 1;
        innerPayload.recursionCount = payload.recursionCount + 1;
        TraceRay(_RTScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, 0, ray, innerPayload);
        
        if(innerPayload.hit == 0)
            payload.color = lightFactor * texColor.xyz;
        else
            payload.color = lightFactor * (reflectivity * innerPayload.color.xyz + (1 - reflectivity) * texColor.xyz);
    }
    else
        payload.color = lightFactor * texColor.xyz;
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.color = float3(0.0, 0.0, 0.0);
    payload.hit = 0;
}