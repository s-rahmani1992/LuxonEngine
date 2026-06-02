#include "AssimpModel3DImporter.h"
#include <vector>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "Mesh.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include <assimp/scene.h>   
#include "Matrix4.h"
#include "Model3DAsset.h"
#include "../StringUtilities.h"
#include <fstream>
#include <Platform/Application.h>
#include <Core/AssetRegistry.h>
#include <JSONUtilities.h>

ref<LuxonEngine::Model3DAsset> LuxonEngine::AssimpModel3DImporter::Import(const std::string& fileName, std::string& error)
{
	Assimp::Importer Importer;
    
	auto pScene = Importer.ReadFile(fileName.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_FlipWindingOrder);

	if (pScene == nullptr) {
		error = "Error Importing 3D Asset File. Error: " + std::string(Importer.GetErrorString());
		return nullptr;
	}

	std::vector<std::pair<std::string, ref<Mesh>>> meshes;
	meshes.reserve(pScene->mNumMeshes);

    std::ifstream metafile(fileName + ".json", std::ios::in | std::ios::binary);

    boost::json::value metaValue;

    if (!metafile) {
        metaValue = CreateDefaultMetaData(pScene, fileName);
    }
    else {
        auto jsonMetaStr = std::string(
            std::istreambuf_iterator<char>(metafile),
            std::istreambuf_iterator<char>()
        );

        boost::system::error_code ec;
        metaValue = boost::json::parse(jsonMetaStr, ec);
    }

    ModelImportProperties transformProps;
    auto& transformObject = metaValue.as_object()["transform"].as_object();
    transformProps.position = JSONToVector3(transformObject["position"].as_object());
    transformProps.axis = JSONToVector3(transformObject["axis"].as_object());
    transformProps.angleDeg = transformObject["angle"].as_double();
    transformProps.scale = JSONToVector3(transformObject["scale"].as_object());

    auto assteRegistry = Platform::Application::GetAssetRegistry();

    auto& meshArray = metaValue.as_object()["meshes"].as_array();
    boost::uuids::string_generator gen;

	for (unsigned int i = 0; i < meshArray.size(); i++) {
        auto& arrayElement = meshArray[i].as_object();
        int index = arrayElement["index"].as_int64();
		const aiMesh* paiMesh = pScene->mMeshes[index];
        auto mesh = CreateMesh(paiMesh, transformProps);
		meshes.push_back(std::make_pair(std::string(paiMesh->mName.C_Str()), mesh));
        assteRegistry->RegisterMesh(gen(arrayElement["uuid"].as_string().c_str()), mesh);
	}

	return std::make_shared<Model3DAsset>(meshes);
}

ref<LuxonEngine::Mesh> LuxonEngine::AssimpModel3DImporter::CreateMesh(const aiMesh* paiMesh, const ModelImportProperties& properties)
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

boost::json::value LuxonEngine::AssimpModel3DImporter::CreateDefaultMetaData(const aiScene* scene, const std::string& filePath)
{
    boost::json::value meta;
    auto& ob = meta.emplace_object();

    auto& transformProps = ob["transform"].emplace_object();
    transformProps.emplace("position", Vector3ToJSON(Vector3(0.0f)));
    transformProps.emplace("axis", Vector3ToJSON(Vector3(0.0f, 1.0f, 0.0f)));
    transformProps.emplace("angle", 0.0f);
    transformProps.emplace("scale", Vector3ToJSON(Vector3(1.0f)));

    auto randomuuidGenerator = boost::uuids::random_generator();
	auto& meshArray = ob["meshes"].emplace_array();
	meshArray.resize(scene->mNumMeshes);

    for (int i = 0; i < scene->mNumMeshes; i++) {
        const aiMesh* paiMesh = scene->mMeshes[i];
		auto& meshObj = meshArray[i].emplace_object();
		meshObj.emplace("name", paiMesh->mName.C_Str());
        meshObj.emplace("index", (INT64)i);
        meshObj.emplace("uuid", boost::uuids::to_string(randomuuidGenerator()));
    }

    std::ofstream file(filePath + ".json");

    file << boost::json::serialize(meta);
    file.close();

    return meta;
}
