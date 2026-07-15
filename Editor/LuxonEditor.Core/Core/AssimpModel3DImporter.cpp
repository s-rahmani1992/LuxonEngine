#include "AssimpModel3DImporter.h"
#include "AssetRegistry.h"
#include <Core/SerializationStream.h>
#include <Core/Mesh.h>
#include <Core/Model3DAsset.h>
#include <Core/Vector2.h>
#include <Core/Vector3.h>
#include <Core/Matrix4.h>
#include <JSONUtilities.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <map>
#include <vector>
#include <string>

#pragma comment(lib, "assimp.lib")

using namespace LuxonEngine;

ref<Model3DAsset> LuxonEditor::AssimpModel3DImporter::Import(const Byte* data, long size, SerializationStream& stream, AssetRegistry* assetRegistry, std::string& error)
{
    Assimp::Importer importer;

    const aiScene* pScene = importer.ReadFileFromMemory(
        data, static_cast<size_t>(size),
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_FlipWindingOrder
    );

    if (pScene == nullptr) {
        error = "Error Importing 3D Asset File. Error: " + std::string(importer.GetErrorString());
        return nullptr;
    }

    ModelImportProperties transformProps;
    SerializationStream transformStream = stream.Object("transform");
	FillPropertiesFromStream(&transformStream, transformProps);
    
    Matrix4 rotationMatrix = Matrix4::Rotate(transformProps.axis, transformProps.angleDeg);
    Matrix4 transformMatrix = Matrix4::Translate(transformProps.position) * Matrix4::Scale(transformProps.scale) * rotationMatrix;

    // Build a map from mesh index -> UUID using the "meshes" array in the stream
    std::map<int, boost::uuids::uuid> meshIndexToGuid;
    {
        auto meshArray = stream.Array("meshes");
        for (auto& meshEntry : meshArray) {
            int index = meshEntry.GetInt("index", -1);
            auto guid = meshEntry.GetGuid("uuid");
            if (index >= 0) {
                meshIndexToGuid[index] = guid;
            }
        }
    }

    boost::uuids::random_generator randomGen;

    std::vector<std::pair<std::string, ref<Mesh>>> meshes;
    meshes.reserve(pScene->mNumMeshes);

    for (unsigned int i = 0; i < pScene->mNumMeshes; i++) {
        const aiMesh* paiMesh = pScene->mMeshes[i];

        std::vector<Vertex> vertices;
        std::vector<UInt32> indices;

        const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

        for (UInt32 v = 0; v < paiMesh->mNumVertices; v++) {
            const aiVector3D* pPos = &(paiMesh->mVertices[v]);
            const aiVector3D* pNormal = &(paiMesh->mNormals[v]);
            const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][v]) : &Zero3D;

            Vertex vert(
                transformMatrix * Vector3(pPos->x, pPos->y, pPos->z),
                Vector2(pTexCoord->x, pTexCoord->y),
                rotationMatrix * Vector3(pNormal->x, pNormal->y, pNormal->z)
            );
            vertices.push_back(vert);
        }

        for (unsigned int f = 0; f < paiMesh->mNumFaces; f++) {
            const aiFace& face = paiMesh->mFaces[f];
            assert(face.mNumIndices == 3);
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }

        auto mesh = std::make_shared<Mesh>(vertices, indices);

        boost::uuids::uuid meshGuid;
        auto it = meshIndexToGuid.find(static_cast<int>(i));
        if (it != meshIndexToGuid.end()) {
            meshGuid = it->second;
        } else {
            meshGuid = randomGen();
        }

        if (assetRegistry != nullptr) {
            assetRegistry->AddMesh(meshGuid, mesh);
        }

        meshes.push_back(std::make_pair(std::string(paiMesh->mName.C_Str()), mesh));
    }

    return std::make_shared<Model3DAsset>(meshes);
}

void LuxonEditor::AssimpModel3DImporter::FillPropertiesFromStream(SerializationStream* stream, ModelImportProperties& properties)
{
    if (stream == nullptr) {
        return;
    }
    //SerializationStream transformStream = stream->Object("transform");
    Vector3 position, axis, scale;
    stream->GetVector3("position", position);
    stream->GetVector3("axis", axis);
    stream->GetVector3("scale", scale);
    properties.position = position;
    properties.axis = axis;
    properties.angleDeg = stream->GetFloat("angle", 0.0f);
	properties.scale = scale;
}

void LuxonEditor::AssimpModel3DImporter::SerializePropertiesToStream(const ModelImportProperties& properties, SerializationStream* stream)
{
    if (stream == nullptr) {
        return;
    }
    stream->SetVector3("position", properties.position);
    stream->SetVector3("axis", properties.axis);
    stream->SetFloat("angle", properties.angleDeg);
	stream->SetVector3("scale", properties.scale);
}
