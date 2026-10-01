#include "Common/TransformStructs.hlsli"
#include "Common/LightStructs.hlsli"

#define MESH_GROUPS_PER_TASK 32
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

struct TaskPayload
{
    float2 planePos;
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
CONSTANT_VARIABLES_END(constantVars, b3)

#define color constantVars.color
#define ambient constantVars.ambient
#define diffuse constantVars.diffuse
#define specular constantVars.specular
#define step constantVars._step
#define totalIndices constantVars._totalIndices

#if defined(_VULKAN)
    StructuredBuffer<MeshVertex> _vertexBuffer;
    StructuredBuffer<uint> _indexBuffer;
#else
    StructuredBuffer<MeshVertex> _vertexBuffer : DX12_REGISTER_SPACE(t0);
    StructuredBuffer<uint> _indexBuffer : DX12_REGISTER_SPACE(t1);
#endif

TEXTURE(_maskTexture, float4, t2)

SAMPLER(mainSampler, s0);

uint GetTriangleCount()
{
    uint indexCount;
    uint indexStride;
    _indexBuffer.GetDimensions(indexCount, indexStride);
    return indexCount / 3;
}

groupshared TaskPayload taskPayload;

// every task group launches up to MESH_GROUPS_PER_TASK mesh groups
[shader("amplification")]
[numthreads(1, 1, 1)]
void as_main(uint3 groupId : SV_GroupID)
{
    // sample at the cell center. task shaders have no derivatives, so the mip level is given explicitly
    float2 uv = (float2(groupId.xy) + 0.5f) / float2(totalIndices);
    float4 texColor = _maskTexture.SampleLevel(mainSampler, uv, 0);
    uint meshGroupCount = texColor.r < 0.1f ? 0 : 1;

    taskPayload.planePos = (uv - 0.5f) * step * totalIndices;
    // DispatchMesh must be reached on every path, a masked out cell launches 0 mesh groups
    DispatchMesh(meshGroupCount, 1, 1, taskPayload);
}

// every mesh group draws TRIANGLES_PER_GROUP triangles of the mesh, one triangle per thread
[shader("mesh")]
[outputtopology("line")]
[numthreads(1, 1, 1)]
void ms_main(
    uint groupThreadId : SV_GroupThreadID,
    uint groupId : SV_GroupID,
    in payload TaskPayload payload,
    out vertices MS_OUTPUT outVertices[2],
    out indices uint2 outLines[1])
{
    SetMeshOutputCounts(2, 1);
    float4x4 mat = mul(cameraData.viewMatrix, cameraData.projectionMatrix);
    float3 entityPosition = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), transformData.modelMatrix).xyz;
    float3 lineStart = mul(float4(payload.planePos.x, 0.0f, payload.planePos.y, 0.0f), transformData.rotationMatrix).xyz + entityPosition;
    float3 lineEnd = mul(float4(payload.planePos.x, 1.0f, payload.planePos.y, 0.0f), transformData.rotationMatrix).xyz + entityPosition;

    MS_OUTPUT vertexOut = (MS_OUTPUT)0;
    vertexOut.worldPos = lineStart;
    vertexOut.pos = mul(float4(lineStart, 1.0f), mat);
    outVertices[0] = vertexOut;

    vertexOut.worldPos = lineEnd;
    vertexOut.pos = mul(float4(lineEnd, 1.0f), mat);
    outVertices[1] = vertexOut;

    outLines[0] = uint2(0, 1);
}

[shader("pixel")]
float4 ps_main(MS_OUTPUT input) : SV_TARGET
{
    return float4(color.xyz, color.w);
}
