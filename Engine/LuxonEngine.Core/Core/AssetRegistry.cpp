#include "AssetRegistry.h"

bool LuxonEngine::AssetRegistry::RegisterTexture2D(const boost::uuids::uuid& guid, const ref<Texture2D>& texture)
{
	auto it = m_textures.emplace(guid, texture);
	return it.second;
}

bool LuxonEngine::AssetRegistry::RegisterMesh(const boost::uuids::uuid& guid, const ref<Mesh>& mesh)
{
	auto it = m_meshes.emplace(guid, mesh);
	return it.second;
}

bool LuxonEngine::AssetRegistry::RegisterMaterial(const boost::uuids::uuid& guid, const ref<Rendering::Material>& material)
{
	auto it = m_materials.emplace(guid, material);
	return it.second;
}

ref<LuxonEngine::Texture2D> LuxonEngine::AssetRegistry::GetTexture2D(const boost::uuids::uuid& guid)
{
	auto it = m_textures.find(guid);

	if (it != m_textures.end())
		return it->second;

	return nullptr;
}

ref<LuxonEngine::Mesh> LuxonEngine::AssetRegistry::GetMesh(const boost::uuids::uuid& guid)
{
	auto it = m_meshes.find(guid);

	if(it != m_meshes.end())
		return it->second;

	return nullptr;
}

ref<LuxonEngine::Rendering::Material> LuxonEngine::AssetRegistry::GetMaterial(const boost::uuids::uuid& guid)
{
	auto it = m_materials.find(guid);

	if (it != m_materials.end())
		return it->second;

	return nullptr;
}
