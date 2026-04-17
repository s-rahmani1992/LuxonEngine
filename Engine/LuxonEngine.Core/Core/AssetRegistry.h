#pragma once
#include "BasicTypes.h"
#include <boost/uuid/uuid.hpp>
#include <map>

namespace LuxonEngine {
	class Texture2D;
	class Mesh;

	namespace Rendering {
		class Material;
	}

	class AssetRegistry {
	public:
		bool RegisterTexture2D(const boost::uuids::uuid& guid, const ref<Texture2D>& texture);
		bool RegisterMesh(const boost::uuids::uuid& guid, const ref<Mesh>& mesh);
		bool RegisterMaterial(const boost::uuids::uuid& guid, const ref<Rendering::Material>& material);
		ref<Texture2D> GetTexture2D(const boost::uuids::uuid& guid);
		ref<Mesh> GetMesh(const boost::uuids::uuid& guid);
		ref<Rendering::Material> GetMaterial(const boost::uuids::uuid& guid);
	private:
		std::map<boost::uuids::uuid, ref<Texture2D>> m_textures;
		std::map<boost::uuids::uuid, ref<Mesh>> m_meshes;
		std::map<boost::uuids::uuid, ref<Rendering::Material>> m_materials;
	};
}