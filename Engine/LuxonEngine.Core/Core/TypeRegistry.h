#pragma once
#include "TypeId.h"
#include "../export.h"
#include <string>

namespace LuxonEngine {
	struct TypeInfo {
		TypeId id = 0;
		std::string name;
	};

	class LUXON_CORE_API TypeRegistry {
	public:
		explicit TypeRegistry(const char* familyName);
		~TypeRegistry();

		TypeRegistry(const TypeRegistry&) = delete;
		TypeRegistry& operator=(const TypeRegistry&) = delete;

		bool Register(TypeId id, const char* name);

		template<typename T>
		bool Register() { return Register(TypeIdOf<T>(), NullTerminatedTypeName<T>()); }

		const TypeInfo* Find(TypeId id) const;

	private:
		struct Impl;
		Impl* m_impl;
	};
}
