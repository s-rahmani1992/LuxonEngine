#include "Common/RTStructs.hlsli"
#include "Common/TransformStructs.hlsli"
#include "Common/LightStructs.hlsli"

CONSTANT_VARIABLES_BEGIN
    uint castShadow;
    float ambient;
    float diffuse;
    float specular;
    uint _missIndex;
CONSTANT_VARIABLES_END(constantVars, b0)

#define castShadow constantVars.castShadow
#define ambient constantVars.ambient
#define diffuse constantVars.diffuse
#define specular constantVars.specular
#define _missIndex constantVars._missIndex

OBJECT_TRANSFORM_VAR(b1)

CAMERA_VAR(b2)

LIGHT_VAR(b3)

RT_SCENE_VAR(t0);

RT_OBJECT_INDEX_BUFFER_VAR(t1)

RT_OBJECT_VERTEX_BUFFER_VAR(t2)

TEXTURE(mainTexture, float4, t3)

SAMPLER(mainSampler, s0)

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
    
    GeneralPayload innerPayload;
    innerPayload.targetMode = 2;
    innerPayload.recursionCount = payload.recursionCount + 1;
    
    float3 lightColor = float3(0.0f, 0.0f, 0.0f);
    float3 ads = float3(ambient, diffuse, specular);
    
    for (uint i = 0; i < lightData.directionalLightCount; i++)
    {
        DirectionalLight light = lightData.directionalLights[i];
        
        RayDesc ray;
        ray.Origin = position;
        ray.Direction = -light.direction;
        ray.TMin = 0.1;
        ray.TMax = 100000;
        TraceRay(_RTScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, _missIndex, ray, innerPayload);

        if (innerPayload.hit > 0)
            lightColor += light.intensity * ambient * light.color.xyz;
        else
            lightColor += PhongDirectionalLight(light, cameraData.position, position, normal, ads);
    }
    
    for (uint i = 0; i < lightData.pointLightCount; i++)
    {
        PointLight light = lightData.pointLights[i];
        
        float d = distance(light.position, position);
        if (d > light.radius)
        {
            lightColor += ambient * light.color.xyz;
            continue;
        }
        
        RayDesc ray;
        ray.Origin = position;
        ray.Direction = normalize(light.position - ray.Origin);
        ray.TMin = 0.1;
        ray.TMax = min(lightData.pointLights[i].radius, d);
        TraceRay(_RTScene, 0 /*rayFlags*/, 0xFF, 0 /* ray index*/, 0, _missIndex, ray, innerPayload);

        if (innerPayload.hit > 0)
            lightColor += ambient * light.color.xyz;
        else
            lightColor += PhongPointLight(light, cameraData.position, position, normal, ads);
    }
    
    payload.color = lightColor * (texColor.xyz);
}

[shader("miss")]
void miss(inout GeneralPayload payload)
{
    payload.hit = 0;
}