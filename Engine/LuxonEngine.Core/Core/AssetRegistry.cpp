#include "AssetRegistry.h"

bool LuxonEngine::AssetRegistry::RegisterTexture2D(const boost::uuids::uuid& guid, const ref<Texture2D>& texture)
{
	auto it = m_textures.emplace(guid, texture);
	return it.second;
}

ref<LuxonEngine::Texture2D> LuxonEngine::AssetRegistry::GetTexture2D(const boost::uuids::uuid& guid)
{
	auto it = m_textures.find(guid);

	if (it != m_textures.end())
		return it->second;

	return nullptr;
}
