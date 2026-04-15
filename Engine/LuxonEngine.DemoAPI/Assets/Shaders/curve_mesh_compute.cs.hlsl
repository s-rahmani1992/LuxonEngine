#include "Common/VariableMacros.hlsli"

struct SplineVertex
{
    float3 pos;
    float2 texCoord;
    float3 norm;
    float3 tangent;
};

float LengthIntergal(float3 startPoint, float3 midPoint, float3 endPoint, float t)
{
    float3 qt = 2.0f * startPoint - 4.0f * midPoint + 2.0f * endPoint;
    float3 q = -2.0f * startPoint + 2.0f * midPoint;
    float A = dot(qt, qt);
    float B = 2.0f * dot(qt, q);
    float C = dot(q, q);
    float h = B / (2 * A);
    float k = C - ((B * B) / (4 * A));
    float exp = sqrt((A * t * t) + (B * t) + C);
    return (0.5f * (t + h) * exp) +
		((0.5f * k) / sqrt(A)) * log((t + h) + exp / sqrt(A));
}

float Length(float3 startPoint, float3 midPoint, float3 endPoint, float t)
{
    return LengthIntergal(startPoint, midPoint, endPoint, t) - LengthIntergal(startPoint, midPoint, endPoint, 0.0f);
}

void Interpolate(float3 startPoint, float3 midPoint, float3 endPoint, float t, out float3 position, out float3 tangent)
{
    float u = 1.0f - t;
    position = u * u * startPoint + 2.0f * u * t * midPoint + t * t * endPoint;
    tangent = normalize(2.0f * (u * (midPoint - startPoint) + t * (endPoint - midPoint)));
}

CONSTANT_VARIABLES_BEGIN
    float3 startPoint;
    float width;
    float3 midPoint;
    float length;
    float3 endPoint;
CONSTANT_VARIABLES_END(CurveProps, b1)

#define _startPoint CurveProps.startPoint
#define _width CurveProps.width
#define _midPoint CurveProps.midPoint
#define _length CurveProps.length
#define _endPoint CurveProps.endPoint

RWStructuredBuffer<SplineVertex> _vertexBuffer : register(u0);

[numthreads(32, 1, 1)]
void cs_main( uint3 DTid : SV_DispatchThreadID )
{
    uint vertexCount;
    uint stride;
    _vertexBuffer.GetDimensions(vertexCount, stride);
    float t = (float) DTid.x / (vertexCount - 1);
    Interpolate(_startPoint, _midPoint, _endPoint, t, _vertexBuffer[DTid.x].pos, _vertexBuffer[DTid.x].tangent);
    
    float3 normal = float3(0.0f, 1.0f, 0.0f);
    float3 bitangent = dot(_vertexBuffer[DTid.x].tangent, normal) * _vertexBuffer[DTid.x].tangent;
    _vertexBuffer[DTid.x].norm = normalize(normal - bitangent);

    _vertexBuffer[DTid.x].texCoord = float2(Length(_startPoint, _midPoint, _endPoint, t) / _length, 0.0f);
}