#include "TransformStructs.hlsli"
#include "LightStructs.hlsli"

#define TRIANGLES_PER_GROUP 16
#define MAX_VERTICES (TRIANGLES_PER_GROUP * 3)
#define MAX_PRIMITIVES TRIANGLES_PER_GROUP

// Matches LuxonEngine::Vertex (Core/Mesh.h), 32 bytes.
struct MeshVertex
{
    float3 pos;
    float2 texCoord;
    float3 norm;
};

struct TaskPayload
{
    float2 planePos;
    uint meshIndex;
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
    float _step;
    uint2 _totalIndices;
    float _entityScale;
    float _distance;
CONSTANT_VARIABLES_END(constantVars, b3)

#define color constantVars.color
#define ambient constantVars.ambient
#define diffuse constantVars.diffuse
#define specular constantVars.specular
#define step constantVars._step
#define totalIndices constantVars._totalIndices
#define instanceScale constantVars._entityScale
#define _distance constantVars._distance

// level 0 is the near mesh, level 1 the far mesh.
#if defined(_VULKAN)
    StructuredBuffer<MeshVertex> _nearVertexBuffer;
    StructuredBuffer<uint> _nearIndexBuffer;
    StructuredBuffer<MeshVertex> _farVertexBuffer;
    StructuredBuffer<uint> _farIndexBuffer;
#else
    StructuredBuffer<MeshVertex> _nearVertexBuffer : DX12_REGISTER_SPACE(t0);
    StructuredBuffer<uint> _nearIndexBuffer : DX12_REGISTER_SPACE(t1);
    StructuredBuffer<MeshVertex> _farVertexBuffer : DX12_REGISTER_SPACE(t4);
    StructuredBuffer<uint> _farIndexBuffer : DX12_REGISTER_SPACE(t5);
#endif

TEXTURE(_maskTexture, float4, t2)

TEXTURE(mainTexture, float4, t3)

SAMPLER(mainSampler, s0);

uint GetTriangleCount(uint meshIndex)
{
    uint indexCount;
    uint indexStride;

    if (meshIndex == 0)
        _nearIndexBuffer.GetDimensions(indexCount, indexStride);
    else
        _farIndexBuffer.GetDimensions(indexCount, indexStride);

    return indexCount / 3;
}

MeshVertex LoadVertex(uint meshIndex, uint index)
{
    if (meshIndex == 0)
        return _nearVertexBuffer[_nearIndexBuffer[index]];

    return _farVertexBuffer[_farIndexBuffer[index]];
}

// instance mesh vertex -> world. the instance is scaled, moved to its cell on the plane, then rotated and
// moved with the entity. planePos is already in world units, so the model matrix (with the plane scale) is not used
MS_OUTPUT TransformInstanceVertex(MeshVertex vertex, float2 planePos, float3 entityPosition, float4x4 viewProjection)
{
    float3 planeLocal = vertex.pos * instanceScale + float3(planePos.x, 0.0f, planePos.y);
    float3 worldPos = mul(float4(planeLocal, 0.0f), transformData.rotationMatrix).xyz + entityPosition;

    MS_OUTPUT vertexOut;
    vertexOut.pos = mul(float4(worldPos, 1.0f), viewProjection);
    vertexOut.texCoord = vertex.texCoord;
    vertexOut.norm = mul(float4(vertex.norm, 0.0f), transformData.rotationMatrix).xyz;
    vertexOut.worldPos = worldPos;
    return vertexOut;
}

groupshared TaskPayload taskPayload;

// one task group per cell, a kept cell launches one mesh group per TRIANGLES_PER_GROUP triangles of the instance mesh
[shader("amplification")]
[numthreads(1, 1, 1)]
void as_main(uint3 groupId : SV_GroupID)
{
    // sample at the cell center. task shaders have no derivatives, so the mip level is given explicitly
    float2 uv = (float2(groupId.xy) + 0.5f) / float2(totalIndices);
    float4 texColor = _maskTexture.SampleLevel(mainSampler, uv, 0);
    taskPayload.planePos = (uv - 0.5f) * step * totalIndices;

    // the level is chosen from the world position of the instance, the plane is rotated and moved with the entity
    float3 entityPosition = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), transformData.modelMatrix).xyz;
    float3 instancePosition = mul(float4(taskPayload.planePos.x, 0.0f, taskPayload.planePos.y, 0.0f), transformData.rotationMatrix).xyz + entityPosition;
    taskPayload.meshIndex = distance(cameraData.position, instancePosition) < _distance ? 0 : 1;

    // the near and far meshes launch different group counts, every task group picks its own
    uint triangleCount = GetTriangleCount(taskPayload.meshIndex);
    uint meshGroupCount = texColor.r < 0.1f ? 0 : (triangleCount + TRIANGLES_PER_GROUP - 1) / TRIANGLES_PER_GROUP;

    // DispatchMesh must be reached on every path, a masked out cell launches 0 mesh groups
    DispatchMesh(meshGroupCount, 1, 1, taskPayload);
}

// every mesh group draws TRIANGLES_PER_GROUP triangles of the instance mesh, one triangle per thread.
// the vertices are not shared, every triangle writes its own 3 vertices
[shader("mesh")]
[outputtopology("triangle")]
[numthreads(TRIANGLES_PER_GROUP, 1, 1)]
void ms_main(
    uint groupThreadId : SV_GroupThreadID,
    uint groupId : SV_GroupID,
    in payload TaskPayload payload,
    out vertices MS_OUTPUT outVertices[MAX_VERTICES],
    out indices uint3 outTriangles[MAX_PRIMITIVES])
{
    // the last group of the mesh usually has fewer triangles
    uint triangleCount = GetTriangleCount(payload.meshIndex);
    uint firstTriangle = groupId * TRIANGLES_PER_GROUP;
    uint groupTriangleCount = firstTriangle < triangleCount ? min(TRIANGLES_PER_GROUP, triangleCount - firstTriangle) : 0;

    SetMeshOutputCounts(groupTriangleCount * 3, groupTriangleCount);

    if (groupThreadId >= groupTriangleCount)
        return;

    float4x4 viewProjection = mul(cameraData.viewMatrix, cameraData.projectionMatrix);
    float3 entityPosition = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), transformData.modelMatrix).xyz;

    uint baseIndex = (firstTriangle + groupThreadId) * 3;
    uint firstVertex = groupThreadId * 3;

    for (uint corner = 0; corner < 3; corner++)
    {
        MeshVertex vertex = LoadVertex(payload.meshIndex, baseIndex + corner);
        outVertices[firstVertex + corner] = TransformInstanceVertex(vertex, payload.planePos, entityPosition, viewProjection);
    }

    outTriangles[groupThreadId] = firstVertex + uint3(0, 1, 2);
}

[shader("pixel")]
float4 ps_main(MS_OUTPUT input) : SV_TARGET
{
    float3 ads = float3(ambient, diffuse, specular);
    float3 lightFactor = PhongLight(lightData, cameraData.position, input.worldPos, input.norm, ads);

    float4 texColor = mainTexture.Sample(mainSampler, input.texCoord);
    return float4(color.xyz * texColor.xyz * lightFactor, color.w);
}
