#include "Model3DAsset.h"

LuxonEngine::Model3DAsset::Model3DAsset(const std::vector<std::pair<std::string, ref<Mesh>>>& meshes)
{
	for(auto& pair: meshes) {
		m_meshes.emplace(pair.first, pair.second);
	}
}
