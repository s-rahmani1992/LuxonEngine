#pragma once
#include <map>
#include <vector>
#include <string>
#include "../BasicTypes.h"

namespace QuantumEngine {
	class Mesh;

	class Model3DAsset {
	public: 
		Model3DAsset(const std::vector<std::pair<std::string, ref<Mesh>>>& meshes);
		Model3DAsset(const Model3DAsset& src) = delete;
		ref<Mesh> GetMesh(const std::string& name) {
			auto it = m_meshes.find(name);
			if (it != m_meshes.end()) {
				return it->second;
			}
			return nullptr;
		}
	private:
		std::map<std::string, ref<Mesh>> m_meshes;
	};
}