#pragma once
#include <boost/uuid/uuid.hpp>

namespace LuxonEditor {
	using GUID = boost::uuids::uuid;

	class __declspec(dllexport) GuidGenerator {
	public:
		static GUID GenerateGUID();
		static GUID GenerateGUIDFromString(const std::string& str);
		static std::string GUIDToString(const GUID& guid);
	};
}