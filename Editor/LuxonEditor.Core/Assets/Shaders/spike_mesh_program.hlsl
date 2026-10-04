#include "TransformStructs.hlsli"
#include "LightStructs.hlsli"

#define MESH_GROUPS_PER_TASK 32
#define TRIANGLES_PER_GROUP 32
#define VERTICES_PER_PYRAMID 4
#define FACES_PER_PYRAMID 4
#define THREADS_PER_GROUP (TRIANGLES_PER_GROUP * FACES_PER_PYRAMID)       // 128
#define MAX_VERTICES (TRIANGLES_PER_GROUP * VERTICES_PER_PYRAMID)          // 128
#define MAX_PRIMITIVES (TRIANGLES_PER_GROUP * FACES_PER_PYRAMID)           // 128

// Matches LuxonEngine::Vertex (Core/Mesh.h), 32 bytes.
struct MeshVertex
{
    float3 pos;
    float2 texCoord;
    float3 norm;
};

struct TaskPayload
{
    uint firstMeshGroup;
};

struct MS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 norm : NORMAL;
    float3 worldPos : POSITION;
};

// DXC marks the mesh shader output as PerPrimitiveEXT but not the matching pixel shader input,
// so on Vulkan the decoration is added by hand (5283 = MeshShadingEXT, 5271 = PerPrimitiveEXT).
struct MS_PRIMITIVE
{
#if defined(_VULKAN)
    [[vk::ext_extension("SPV_EXT_mesh_shader")]] [[vk::ext_capability(5283)]] [[vk::ext_decorate(5271)]]
#endif
    nointerpolation float3 faceNorm : FACE_NORMAL;
};

// Corners of each pyramid face, indexing the pyramid's vertices (0..2 = base corners, 3 = apex).
// Faces 0..2 are the sides (edge + apex) and keep the source triangle's winding. Face 3 is the base
// with reversed winding since it now faces the other way.
static const uint3 PyramidFaces[FACES_PER_PYRAMID] = {
    uint3(0, 1, 3),
    uint3(1, 2, 3),
    uint3(2, 0, 3),
    uint3(0, 2, 1),
};

OBJECT_TRANSFORM_VAR(b0)

CAMERA_VAR(b1)

LIGHT_VAR(b2)

TEXTURE(mainTexture, float4, t2)

SAMPLER(mainSampler, s0);

CONSTANT_VARIABLES_BEGIN
    float4 color;
    float ambient;
    float diffuse;
    float specular;
    float smoothNormals;
    float _height;
CONSTANT_VARIABLES_END(constantVars, b3)

#define color constantVars.color
#define ambient constantVars.ambient
#define diffuse constantVars.diffuse
#define specular constantVars.specular
#define smoothNormals constantVars.smoothNormals
#define _height constantVars._height

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

groupshared TaskPayload taskPayload;

[shader("amplification")]
[numthreads(1, 1, 1)]
void as_main(uint groupId : SV_GroupID)
{
    uint meshGroupCount = (GetTriangleCount() + TRIANGLES_PER_GROUP - 1) / TRIANGLES_PER_GROUP;
    taskPayload.firstMeshGroup = groupId * MESH_GROUPS_PER_TASK;
    uint taskMeshGroupCount = min(MESH_GROUPS_PER_TASK, meshGroupCount - taskPayload.firstMeshGroup);

    DispatchMesh(taskMeshGroupCount, 1, 1, taskPayload);
}

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(THREADS_PER_GROUP, 1, 1)]
void ms_main(
    uint groupThreadId : SV_GroupThreadID,
    uint groupId : SV_GroupID,
    in payload TaskPayload payload,
    out vertices MS_OUTPUT outVertices[MAX_VERTICES],
    out indices uint3 outTriangles[MAX_PRIMITIVES],
    out primitives MS_PRIMITIVE outPrimitives[MAX_PRIMITIVES])
{
    uint triangleCount = GetTriangleCount();
    uint firstTriangle = (payload.firstMeshGroup + groupId) * TRIANGLES_PER_GROUP;
    uint groupTriangleCount = min(TRIANGLES_PER_GROUP, triangleCount - firstTriangle);

    SetMeshOutputCounts(groupTriangleCount * VERTICES_PER_PYRAMID, groupTriangleCount * FACES_PER_PYRAMID);

    uint localTriangle = groupThreadId / FACES_PER_PYRAMID;
    if (localTriangle >= groupTriangleCount)
        return;

    uint baseIndex = (firstTriangle + localTriangle) * 3;
    MeshVertex v0 = _vertexBuffer[_indexBuffer[baseIndex]];
    MeshVertex v1 = _vertexBuffer[_indexBuffer[baseIndex + 1]];
    MeshVertex v2 = _vertexBuffer[_indexBuffer[baseIndex + 2]];

    float3 triangleNormal = normalize(cross(v1.pos - v0.pos, v2.pos - v0.pos));
    float windingSign = dot(triangleNormal, v0.norm + v1.norm + v2.norm) < 0.0f ? -1.0f : 1.0f;

    float3 cornerPos[VERTICES_PER_PYRAMID] = {
        v0.pos, v1.pos, v2.pos,
        (v0.pos + v1.pos + v2.pos) / 3.0f + windingSign * triangleNormal * _height,
    };
    // Base corners keep the source mesh's normals, the apex points along the triangle normal.
    float3 cornerNormal[VERTICES_PER_PYRAMID] = {
        v0.norm, v1.norm, v2.norm,
        windingSign * triangleNormal,
    };
    float2 cornerTexCoord[VERTICES_PER_PYRAMID] = {
        v0.texCoord, v1.texCoord, v2.texCoord,
        (v0.texCoord + v1.texCoord + v2.texCoord) / 3.0f,
    };

    // The 4 threads of a pyramid share its vertices: thread t writes vertex t and face t.
    uint corner = groupThreadId % VERTICES_PER_PYRAMID;
    float3 pos = cornerPos[corner];

    MS_OUTPUT vertexOut;
    vertexOut.pos = mul(float4(pos, 1.0f), mul(transformData.modelViewMatrix, cameraData.projectionMatrix));
    vertexOut.texCoord = cornerTexCoord[corner];
    vertexOut.norm = mul(float4(cornerNormal[corner], 0.0f), transformData.rotationMatrix).xyz;
    vertexOut.worldPos = mul(float4(pos, 1.0f), transformData.modelMatrix).xyz;
    outVertices[groupThreadId] = vertexOut;

    uint3 face = PyramidFaces[groupThreadId % FACES_PER_PYRAMID];
    float3 faceNormal = windingSign * normalize(cross(cornerPos[face.y] - cornerPos[face.x], cornerPos[face.z] - cornerPos[face.x]));

    uint firstPyramidVertex = localTriangle * VERTICES_PER_PYRAMID;
    outTriangles[groupThreadId] = firstPyramidVertex + face;
    outPrimitives[groupThreadId].faceNorm = mul(float4(faceNormal, 0.0f), transformData.rotationMatrix).xyz;
}

[shader("pixel")]
float4 ps_main(MS_OUTPUT input, MS_PRIMITIVE primitive) : SV_TARGET
{
    float3 ads = float3(ambient, diffuse, specular);
    float3 normal = lerp(primitive.faceNorm, normalize(input.norm), smoothNormals);
    float3 lightFactor = PhongLight(lightData, cameraData.position, input.worldPos, normal, ads);
    float4 texColor = mainTexture.Sample(mainSampler, input.texCoord);
    return float4(lightFactor * color.xyz * texColor.xyz, color.w);
}
