#include "Common/TransformStructs.hlsli"
#include "Common/LightStructs.hlsli"

#define TRIANGLES_PER_GROUP 64
#define MAX_VERTICES (TRIANGLES_PER_GROUP * 3)
#define MAX_PRIMITIVES TRIANGLES_PER_GROUP

// Matches LuxonEngine::Vertex (Core/Mesh.h), 32 bytes.
struct MeshVertex
{
    float3 pos;
    float2 texCoord;
    float3 norm;
};

struct MS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 norm : NORMAL;
    float3 worldPos : POSITION;
};

OBJECT_TRANSFORM_VAR(b0)

CAMERA_VAR(b1)

LIGHT_VAR(b2)

CONSTANT_VARIABLES_BEGIN
    float4 color;
    float ambient;
    float diffuse;
    float specular;
CONSTANT_VARIABLES_END(constantVars, b3)

#define color constantVars.color
#define ambient constantVars.ambient
#define diffuse constantVars.diffuse
#define specular constantVars.specular

#if defined(_VULKAN)
    StructuredBuffer<MeshVertex> _vertexBuffer;
    StructuredBuffer<uint> _indexBuffer;
#else
    StructuredBuffer<MeshVertex> _vertexBuffer : DX12_REGISTER_SPACE(t0);
    StructuredBuffer<uint> _indexBuffer : DX12_REGISTER_SPACE(t1);
#endif

uint GetTriangleCount()
{
    uint indexCount;
    uint indexStride;
    _indexBuffer.GetDimensions(indexCount, indexStride);
    return indexCount / 3;
}

// every mesh group draws TRIANGLES_PER_GROUP triangles of the mesh, one triangle per thread
[shader("mesh")]
[outputtopology("triangle")]
[numthreads(TRIANGLES_PER_GROUP, 1, 1)]
void $(MESH_MAIN)(
    uint groupThreadId : SV_GroupThreadID,
    uint groupId : SV_GroupID,
    out vertices MS_OUTPUT outVertices[MAX_VERTICES],
    out indices uint3 outTriangles[MAX_PRIMITIVES])
{
    uint triangleCount = GetTriangleCount();
    uint firstTriangle = groupId * TRIANGLES_PER_GROUP;
    uint groupTriangleCount = firstTriangle < triangleCount ? min(TRIANGLES_PER_GROUP, triangleCount - firstTriangle) : 0;

    SetMeshOutputCounts(groupTriangleCount * 3, groupTriangleCount);

    if (groupThreadId >= groupTriangleCount)
        return;

    uint baseIndex = (firstTriangle + groupThreadId) * 3;
    float4x4 mvp = mul(transformData.modelViewMatrix, cameraData.projectionMatrix);

    for (uint corner = 0; corner < 3; corner++)
    {
        MeshVertex vertex = _vertexBuffer[_indexBuffer[baseIndex + corner]];

        MS_OUTPUT vertexOut;
        vertexOut.pos = mul(float4(vertex.pos, 1.0f), mvp);
        vertexOut.texCoord = vertex.texCoord;
        vertexOut.norm = mul(float4(vertex.norm, 0.0f), transformData.rotationMatrix).xyz;
        vertexOut.worldPos = mul(float4(vertex.pos, 1.0f), transformData.modelMatrix).xyz;
        outVertices[groupThreadId * 3 + corner] = vertexOut;
    }

    outTriangles[groupThreadId] = groupThreadId * 3 + uint3(0, 1, 2);
}

[shader("pixel")]
float4 $(PIXEL_MAIN)(MS_OUTPUT input) : SV_TARGET
{
    float3 ads = float3(ambient, diffuse, specular);
    float3 lightFactor = PhongLight(lightData, cameraData.position, input.worldPos, normalize(input.norm), ads);
    return float4(lightFactor * color.xyz, color.w);
}
