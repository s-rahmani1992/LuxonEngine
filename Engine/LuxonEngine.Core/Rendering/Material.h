#pragma once
#include <string>
#include <map>
#include <set>
#include <vector>
#include "../Core/Color.h"
#include "../BasicTypes.h"
#include "../export.h"
#include <boost/uuid/uuid.hpp>

namespace LuxonEngine {
	class Texture2D;
	namespace Rendering {
		class ShaderProgram;
		struct ShaderVariableReflection;
	}
}

namespace LuxonEngine::Rendering {
	struct MaterialValueData {
		UInt32 fieldIndex;
		UInt32 size;
		Byte* data;
	};
	struct MaterialTextureData {
		UInt32 fieldIndex;
		ref<Texture2D> texture;
	};

	struct MaterialValueFieldInfo {
		std::string name;
		UInt32 fieldIndex;
		UInt32 size;
	};

	struct MaterialTextureFieldInfo {
		std::string name;
		UInt32 fieldIndex;
	};

	struct MaterialReflection {
		std::vector<MaterialValueFieldInfo> valueFields;
		std::vector<MaterialTextureFieldInfo> textureFields;
	};

	class LUXON_CORE_API Material {
	public:
		Material(const ref<ShaderProgram>& program);

		Material(const ref<ShaderProgram>& program, const MaterialReflection* fields);
		
		Material& operator=(Material& srcMaterial);

		virtual ~Material();

		/// <summary>
		/// Gets the shader program that this material is created from
		/// </summary>
		/// <returns></returns>
		ref<ShaderProgram> GetProgram() { return m_program; }

		void SetProgram(const ref<ShaderProgram>& program) { m_program = program; }

		/// <summary>
		/// Sets the value of a field in the material. the value type must be a simple type such as int, color, etc.
		/// </summary>
		/// <typeparam name="T">type of value</typeparam>
		/// <param name="fieldName">name of the field</param>
		/// <param name="value">value data</param>
		template<typename T>
		void SetValue(const std::string& fieldName, const T& value) {
			auto it = m_valueFields.find(fieldName);
			if (it != m_valueFields.end()) {
				MaterialValueData& valueData = it->second;
				if (sizeof(T) == valueData.size) {
					memcpy(valueData.data, &value, sizeof(T));
					m_modifiedValues.emplace(&valueData);
				}
			}
		}

		void SetValue(const std::string& fieldName, void* src, UInt32 size);

		/// <summary>
		/// Sets the value of a field in the material. the value type must be a simple type such as int, color, etc.
		/// </summary>
		/// <typeparam name="T">type of value</typeparam>
		/// <param name="fieldName">name of the field</param>
		/// <param name="value">value data</param>
		template<typename T>
		T GetValue(const std::string& fieldName, const T& defaultValue) {
			auto it = m_valueFields.find(fieldName);
			if (it == m_valueFields.end()) {
				return defaultValue;
			}

			MaterialValueData& valueData = it->second;
			T* data = (T*)valueData.data;
			return *data;
		}

		/// <summary>
		/// Sets the texture of a texture field in the material.
		/// </summary>
		/// <param name="fieldName">name of the field</param>
		/// <param name="texture">texture asset</param>
		void SetTexture2D(const std::string& fieldName, const ref<Texture2D>& texture);

		/// <summary>
		/// Gets the location of a value associated with field name. returns nullptr if field not found.
		/// </summary>
		/// <param name="fieldName">name of the field</param>
		/// <returns></returns>
		Byte* GetValueLocation(const std::string& fieldName);

		inline std::map<std::string, MaterialTextureData>* GetTextureFields() {
			return &m_textureFields;
		}

		inline std::map<std::string, MaterialValueData>* GetValueFields() {
			return &m_valueFields;
		}

		inline UInt32 GetTextureFieldCount() const {
			return static_cast<UInt32>(m_textureFields.size());
		}

		inline const std::set<MaterialTextureData*>& GetModifiedTextures() const {
			return m_modifiedTextures;
		}

		inline const std::set<MaterialValueData*>& GetModifiedValues() const {
			return m_modifiedValues;
		}

		inline void ClearModifiedTextures() {
			m_modifiedTextures.clear();
			m_modifiedValues.clear();
		}

		inline void ClearTextures() { m_modifiedTextures.clear(); }
		inline void ClearModifiedValues() { m_modifiedValues.clear(); }

		const boost::uuids::uuid& GetProgramGuid() const { return m_programGuid; }
		void SetProgramGuid(const boost::uuids::uuid& guid) { m_programGuid = guid; }

	private:
		void InitializeFields(const ShaderVariableReflection& reflection);

	protected:
		ref<ShaderProgram> m_program;
	private:
		Byte* m_valueData = nullptr; // contiguous array holding all value data
		std::map<std::string, MaterialValueData> m_valueFields;
		std::map<std::string, MaterialTextureData> m_textureFields;
		std::set<MaterialTextureData*> m_modifiedTextures;
		std::set<MaterialValueData*> m_modifiedValues;

		boost::uuids::uuid m_programGuid;
	};
}