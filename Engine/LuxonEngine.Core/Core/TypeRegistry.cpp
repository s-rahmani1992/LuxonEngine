#include "TypeRegistry.h"
#include "Logger.h"
#include <map>
#include <mutex>

namespace LuxonEngine {
	namespace TypeNameCheck {
		namespace A { class Foo {}; }
		namespace B { class Foo {}; }
	}

	static_assert(QualifiedTypeName<TypeNameCheck::A::Foo>() == "LuxonEngine::TypeNameCheck::A::Foo", "The qualified type name cannot be read with this compiler");
	static_assert(QualifiedTypeName<TypeNameCheck::A::Foo>() != QualifiedTypeName<TypeNameCheck::B::Foo>(), "Types in different namespaces must have different names");
	static_assert(TypeIdOf<TypeNameCheck::A::Foo>() != TypeIdOf<TypeNameCheck::B::Foo>(), "Types in different namespaces must have different ids");

	struct TypeRegistry::Impl {
		std::string familyName;
		mutable std::mutex mutex;
		std::map<TypeId, TypeInfo> types; // map nodes are stable, so the pointers returned by Find stay valid
	};

	TypeRegistry::TypeRegistry(const char* familyName)
		: m_impl(new Impl{ familyName })
	{
	}

	TypeRegistry::~TypeRegistry()
	{
		delete m_impl;
	}

	bool TypeRegistry::Register(TypeId id, const char* name)
	{
		std::lock_guard<std::mutex> lock(m_impl->mutex);

		auto it = m_impl->types.find(id);

		if (it == m_impl->types.end()) {
			m_impl->types.emplace(id, TypeInfo{ id, name });
			return true;
		}

		if (it->second.name == name)
			return true;

		Logger::LogError("The " + m_impl->familyName + " types \"" + it->second.name + "\" and \"" + name + "\" have the same type id, the second one is not registered");
		return false;
	}

	const TypeInfo* TypeRegistry::Find(TypeId id) const
	{
		std::lock_guard<std::mutex> lock(m_impl->mutex);

		auto it = m_impl->types.find(id);
		return it == m_impl->types.end() ? nullptr : &it->second;
	}
}
