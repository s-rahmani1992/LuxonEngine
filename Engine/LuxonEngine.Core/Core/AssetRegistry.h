#pragma once
#include "BasicTypes.h"
#include <boost/uuid/uuid.hpp>
#include <map>

namespace LuxonEngine {
	class Texture2D;

	class AssetRegistry {
	public:
		bool RegisterTexture2D(const boost::uuids::uuid& guid, const ref<Texture2D>& texture);
		ref<Texture2D> GetTexture2D(const boost::uuids::uuid& guid);

	private:
		std::map<boost::uuids::uuid, ref<Texture2D>> m_textures;
	};
}