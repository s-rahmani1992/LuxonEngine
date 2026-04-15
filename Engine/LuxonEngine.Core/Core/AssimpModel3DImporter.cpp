#include "AssimpModel3DImporter.h"
#include <vector>
#include "Mesh.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"   
#include "Matrix4.h"
#include "Model3DAsset.h"
#include "../StringUtilities.h"

ref<QuantumEngine::Model3DAsset> QuantumEngine::AssimpModel3DImporter::Import(const std::string& fileName, const ModelImportProperties& properties, std::string& error)
{
	Assimp::Importer Importer;

	auto pScene = Importer.ReadFile(fileName.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_FlipWindingOrder);

	if (pScene == nullptr) {
		error = "Error Importing 3D Asset File. Error: " + std::string(Importer.GetErrorString());
		return nullptr;
	}

	std::vector<std::pair<std::string, ref<Mesh>>> meshes;
	meshes.reserve(pScene->mNumMeshes);

	for (unsigned int i = 0; i < pScene->mNumMeshes; i++) {
		const aiMesh* paiMesh = pScene->mMeshes[i];
        auto mesh = CreateMesh(paiMesh, properties);
		meshes.push_back(std::make_pair(std::string(paiMesh->mName.C_Str()), mesh));
	}

	return std::make_shared<Model3DAsset>(meshes);
}

ref<QuantumEngine::Mesh> QuantumEngine::AssimpModel3DImporter::CreateMesh(const aiMesh* paiMesh, const ModelImportProperties& properties)
{
    std::vector<Vertex> vertices;
    std::vector<UInt32> indices;

    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
    Matrix4 rotationMatrix = Matrix4::Rotate(properties.axis, properties.angleDeg);
	Matrix4 transformMatrix = Matrix4::Translate(properties.position) * Matrix4::Scale(properties.scale) * rotationMatrix;
    
    for (UInt32 i = 0; i < paiMesh->mNumVertices; i++) {
        const aiVector3D* pPos = &(paiMesh->mVertices[i]);
        const aiVector3D* pNormal = &(paiMesh->mNormals[i]);
        const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;

        Vertex v(transformMatrix * Vector3(pPos->x, pPos->y, pPos->z),
            Vector2(pTexCoord->x, pTexCoord->y),
            rotationMatrix * Vector3(pNormal->x, pNormal->y, pNormal->z));

        vertices.push_back(v);
    }

    for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
        const aiFace& Face = paiMesh->mFaces[i];
        assert(Face.mNumIndices == 3);
        indices.push_back(Face.mIndices[0]);
        indices.push_back(Face.mIndices[1]);
        indices.push_back(Face.mIndices[2]);
    }

	return std::make_shared<Mesh>(vertices, indices);
}
