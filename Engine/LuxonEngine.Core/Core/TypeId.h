#pragma once
#include "../BasicTypes.h"
#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace LuxonEngine {
	using TypeId = UInt64;

	constexpr TypeId HashTypeName(std::string_view name)
	{
		TypeId hash = 14695981039346656037ull;

		for (char character : name) {
			hash ^= static_cast<UInt8>(character);
			hash *= 1099511628211ull;
		}

		return hash;
	}

	namespace TypeIdDetail {
		template<typename T>
		constexpr std::string_view RawTypeSignature()
		{
#if defined(_MSC_VER)
			return __FUNCSIG__;
#else
			return __PRETTY_FUNCTION__;
#endif
		}
	}

	template<typename T>
	constexpr std::string_view QualifiedTypeName()
	{
		std::string_view signature = TypeIdDetail::RawTypeSignature<T>();

#if defined(_MSC_VER)
		constexpr std::string_view prefix = "RawTypeSignature<";
		constexpr std::string_view suffix = ">(void)";
		signature.remove_prefix(signature.find(prefix) + prefix.size());
		signature.remove_suffix(signature.size() - signature.rfind(suffix));

		if (signature.starts_with("class "))
			signature.remove_prefix(6);
		else if (signature.starts_with("struct "))
			signature.remove_prefix(7);
#else
		constexpr std::string_view prefix = "T = ";
		signature.remove_prefix(signature.find(prefix) + prefix.size());
		signature = signature.substr(0, signature.find_first_of(";]"));
#endif
		return signature;
	}

	template<typename T>
	constexpr TypeId TypeIdOf()
	{
		return HashTypeName(QualifiedTypeName<T>());
	}

	namespace TypeIdDetail {
		template<size_t Size>
		constexpr std::array<char, Size + 1> MakeNullTerminated(std::string_view text)
		{
			std::array<char, Size + 1> result{};

			for (size_t i = 0; i < Size; i++)
				result[i] = text[i];

			return result;
		}

		template<typename T>
		struct TypeNameStorage {
			static constexpr std::string_view name = QualifiedTypeName<T>();
			static constexpr std::array<char, name.size() + 1> value = MakeNullTerminated<name.size()>(name);
		};
	}

	template<typename T>
	constexpr const char* NullTerminatedTypeName()
	{
		return TypeIdDetail::TypeNameStorage<T>::value.data();
	}
}

#define LUXON_TYPE_STATIC(Type) \
	static constexpr ::LuxonEngine::TypeId StaticTypeId() { return ::LuxonEngine::TypeIdOf<Type>(); } \
	static constexpr const char* StaticTypeName() { return ::LuxonEngine::NullTerminatedTypeName<Type>(); }

#define LUXON_ASSERT_SELF_TYPE(Type) \
	static_assert(std::is_same_v<Type, std::remove_const_t<std::remove_pointer_t<decltype(this)>>>, "The type given to the macro must be the class it is used in")
