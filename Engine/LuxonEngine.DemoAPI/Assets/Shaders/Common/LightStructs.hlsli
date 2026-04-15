#include "VariableMacros.hlsli"

struct Attenuation
{
    float c0;
    float c1;
    float c2;
    float AttenuationFactor(float distance)
    {
        return 1.0f / (c0 + c1 * distance + c2 * distance * distance);
    }
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
    Attenuation attenuation;
    float radius;
};

struct LightData
{
    DirectionalLight directionalLights[10];
    PointLight pointLights[10];
    uint directionalLightCount;
    uint pointLightCount;
};

inline float3 PhongDirectionalLight(DirectionalLight light, float3 camPosition, float3 position, float3 normal, float3 ads)
{
    float3 norm = normalize(normal);
    float3 lightDir = normalize(light.direction);

    float diff = max(dot(norm, -lightDir), 0.0);
    float diffuse = diff * ads.y;

    float specular = 0;
    
    if (diff > 0.0f)
    {
        float3 viewDir = normalize(camPosition - position);
        float3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        specular = spec * ads.z;
    }
    

    return (ads.x + light.intensity * (specular + diffuse)) * light.color.xyz;
}

inline float3 PhongPointLight(PointLight light, float3 camPosition, float3 position, float3 normal, float3 ads)
{
    float3 norm = normalize(normal);
    float3 lightDir = -position + light.position;
    float sqrlightMag = dot(lightDir, lightDir);

    if (sqrlightMag > pow(light.radius, 2))
    { //light is too far away from pixel
        return ads.x * light.color.xyz;
    }
	
    float lightMag = sqrt(sqrlightMag);
    lightDir = normalize(lightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    float diffuse = diff * ads.y;

    float specular = 0;
    
    if (diff > 0.0f)
    {
        float3 viewDir = normalize(camPosition - position);
        float3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        specular = spec * ads.z;
    }
    
    float att = light.attenuation.AttenuationFactor(lightMag);
    return (ads.x + att * light.intensity * (specular + diffuse)) * light.color.xyz;
}

inline float3 PhongLight(LightData lightData, float3 camPosition, float3 position, float3 normal, float3 ads)
{
    float3 lightFactor = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < lightData.directionalLightCount; i++)
        lightFactor += PhongDirectionalLight(lightData.directionalLights[i], camPosition, position, normal, ads);
    for (uint i = 0; i < lightData.pointLightCount; i++)
        lightFactor += PhongPointLight(lightData.pointLights[i], camPosition, position, normal, ads);
    
    return lightFactor;
}

#if defined(_VULKAN)
    #define LIGHT_VAR(x)  cbuffer _LightData{ \
        LightData lightData; \
    };
#else
    #define LIGHT_VAR(x)  cbuffer _LightData : DX12_REGISTER_SPACE(x) {  \
        LightData lightData; \
    };
#endif
