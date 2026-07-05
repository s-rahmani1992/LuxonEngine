#pragma once
#include <boost/uuid/uuid.hpp>

namespace LuxonEditor {
	using GUID = boost::uuids::uuid;

	class GuidGenerator {
	public:
		static GUID GenerateGUID();
	};
}