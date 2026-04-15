#pragma once
#include <string>
#include <map>
#include <set>
#include <vector>
#include "../Core/Color.h"
#include "../BasicTypes.h"

namespace QuantumEngine {
	class Texture2D;
	namespace Rendering {
		class ShaderProgram;
	}
}

namespace QuantumEngine::Rendering {
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

	class Material {
	public:
		Material(const ref<ShaderProgram>& program) 
			:m_program(program)
		{

		}

		Material(const ref<ShaderProgram>& program, const MaterialReflection* fields)
			:m_program(program)
		{
			// Allocate Array holding Value Data
			UInt32 totalValueSize = 0;
			for (auto& valueField : fields->valueFields) {
				totalValueSize += valueField.size;
			}
			m_valueData = new Byte[totalValueSize]();

			// Initialize Value Array and Map
			totalValueSize = 0;
			for(auto& valueField : fields->valueFields) {
				m_valueFields[valueField.name] = MaterialValueData{
					.fieldIndex = valueField.fieldIndex,
					.size = valueField.size,
					.data = m_valueData + totalValueSize,
				};

				totalValueSize += valueField.size;
			}

			UInt32 index = 0;
			// Initialize Texture Map
			for (auto& textureField : fields->textureFields) {
				m_textureFields[textureField.name] = MaterialTextureData{
					.fieldIndex = index,
					.texture = nullptr,
				};

				index++;
			}
		}
		
		virtual ~Material() {
			delete[] m_valueData;
		}

		/// <summary>
		/// Gets the shader program that this material is created from
		/// </summary>
		/// <returns></returns>
		ref<ShaderProgram> GetProgram() { return m_program; }

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
		void SetTexture2D(const std::string& fieldName, const ref<Texture2D>& texture) {
			auto it = m_textureFields.find(fieldName);
			if (it != m_textureFields.end()) {
				MaterialTextureData& textureData = it->second;
				textureData.texture = texture;
				m_modifiedTextures.emplace(&textureData);
			}
		}

		/// <summary>
		/// Gets the location of a value associated with field name. returns nullptr if field not found.
		/// </summary>
		/// <param name="fieldName">name of the field</param>
		/// <returns></returns>
		Byte* GetValueLocation(const std::string& fieldName) {
			auto it = m_valueFields.find(fieldName);

			if (it != m_valueFields.end()) {
				MaterialValueData& valueData = it->second;
				return valueData.data;
			}

			return nullptr;
		}

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

	protected:
		ref<ShaderProgram> m_program;
	private:
		Byte* m_valueData = nullptr; // contiguous array holding all value data
		std::map<std::string, MaterialValueData> m_valueFields;
		std::map<std::string, MaterialTextureData> m_textureFields;
		std::set<MaterialTextureData*> m_modifiedTextures;
		std::set<MaterialValueData*> m_modifiedValues;
	};
}