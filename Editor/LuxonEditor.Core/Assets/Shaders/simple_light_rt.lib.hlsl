#include "Common/VariableMacros.hlsli"
#include "Common/TransformStructs.hlsli"
#include "Common/LightStructs.hlsli"
#include "Common/RTStructs.hlsli"

CONSTANT_VARIABLES_BEGIN
    float ambient;
    float diffuse;
    float specular;
    uint castShadow;
CONSTANT_VARIABLES_END(constantVars, b0)

#define ambient constantVars.ambient
#define diffuse constantVars.diffuse
#define specular constantVars.specular
#define castShadow constantVars.castShadow

OBJECT_TRANSFORM_VAR(b1)

CAMERA_VAR(b2)

LIGHT_VAR(b3)

TEXTURE(mainTexture, float4, t0)

SAMPLER(mainSampler, s0)

RT_OBJECT_INDEX_BUFFER_VAR(t1)

RT_OBJECT_VERTEX_BUFFER_VAR(t2)

[shader("closesthit")]
void chs(inout GeneralPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    if (payload.targetMode == 2 && castShadow > 0)
    {
        payload.hit = 1;
        return;
    }
    
    float2 uv = CalculateUV(_indexBuffer, _vertexBuffer, attribs.barycentrics);
    float3 normal = CalculateNormal(_indexBuffer, _vertexBuffer, attribs.barycentrics);
    normal = mul(float4(normal, 1.0f), transformData.rotationMatrix).xyz;
    float3 position = CalculeteHitPosition();
    
    float4 texColor = mainTexture.SampleLevel(mainSampler, uv, 0);

    float3 ads = float3(ambient, diffuse, specular);
    float3 lightFactor = PhongLight(lightData, cameraData.position, position, normal, ads);
    
    payload.color = lightFactor * texColor.xyz;
    payload.hit = 1;
}