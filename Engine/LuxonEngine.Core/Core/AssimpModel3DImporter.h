#pragma once
#include <string>
#include <Platform/CommonWin.h>
#include "../BasicTypes.h"
#include "Vector3.h"
#include <boost/json.hpp>

struct aiMesh;
struct aiScene;

namespace LuxonEngine {
	class Model3DAsset;
	class Mesh;

	struct ModelImportProperties{
		Vector3 position = Vector3(0.0f);
		Vector3 axis = Vector3(0.0f, 0.0f, 1.0f);
		Float angleDeg = 0.0f;
		Vector3 scale = Vector3(1.0f);
	};

	class AssimpModel3DImporter {
	public:
		static ref<Model3DAsset> Import(const std::string& filePath, std::string& error);
	private:
		static ref<Mesh> CreateMesh(const aiMesh* paiMesh, const ModelImportProperties& properties);
		static boost::json::value CreateDefaultMetaData(const aiScene* scene, const std::string& filePath);
	};
}