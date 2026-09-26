#include "Material.h"
#include "ShaderProgram.h"

namespace LuxonEngine::Rendering
{
	Material::Material(const ref<ShaderProgram>& program)
		:m_program(program)
	{
		if (program != nullptr)
			InitializeFields(program->GetVariableReflection());
	}

	Material::Material(const ref<ShaderProgram>& program, const MaterialReflection* fields)
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
		for (auto& valueField : fields->valueFields) {
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

	Material::~Material()
	{
		if (m_valueData != nullptr)
			delete[] m_valueData;
	}

	Material& LuxonEngine::Rendering::Material::operator=(Material& srcMaterial)
	{
		delete[] m_valueData;

		m_valueData = srcMaterial.m_valueData;
		m_programGuid = srcMaterial.m_programGuid;
		m_program = srcMaterial.m_program;
		m_valueFields = srcMaterial.m_valueFields;
		m_textureFields = srcMaterial.m_textureFields;
		srcMaterial.m_valueData = nullptr;
		m_modifiedTextures.clear();
		m_modifiedValues.clear();
		return *this;
	}

	void Material::SetValue(const std::string& fieldName, void* src, UInt32 size)
	{
		auto it = m_valueFields.find(fieldName);

		if (it == m_valueFields.end())
			return;

		if (it->second.size != size)
			return;

		MaterialValueData& valueData = it->second;
		memcpy(valueData.data, src, size);
		m_modifiedValues.emplace(&valueData);
	}

	void Material::SetTexture2D(const std::string& fieldName, const ref<Texture2D>& texture)
	{
		auto it = m_textureFields.find(fieldName);
		if (it != m_textureFields.end()) {
			MaterialTextureData& textureData = it->second;
			textureData.texture = texture;
			m_modifiedTextures.emplace(&textureData);
		}
	}

	Byte* Material::GetValueLocation(const std::string& fieldName)
	{
		auto it = m_valueFields.find(fieldName);

		if (it != m_valueFields.end()) {
			MaterialValueData& valueData = it->second;
			return valueData.data;
		}

		return nullptr;
	}

	void Material::InitializeFields(const ShaderVariableReflection& reflection)
	{
		// Allocate Array holding Value Data. Value fields are from the constant data
		UInt32 totalValueSize = 0;
		for (auto& block : reflection.constants.blocks) {
			if (block.isInternal)
				continue;

			for (auto& variable : block.variables)
				totalValueSize += variable.size;
		}
		m_valueData = new Byte[totalValueSize]();

		// Initialize Value Array and Map
		totalValueSize = 0;
		UInt32 index = 0;
		for (auto& block : reflection.constants.blocks) {
			if (block.isInternal)
				continue;

			for (auto& variable : block.variables) {
				m_valueFields[variable.name] = MaterialValueData{
					.fieldIndex = index,
					.size = variable.size,
					.data = m_valueData + totalValueSize,
				};

				totalValueSize += variable.size;
				index++;
			}
		}

		// Initialize Texture Map. Only textures are considered for material texture fields
		index = 0;
		for (auto& resource : reflection.resources) {
			if (resource.isInternal || resource.kind != ShaderResourceKind::Texture)
				continue;

			m_textureFields[resource.name] = MaterialTextureData{
				.fieldIndex = index,
				.texture = nullptr,
			};

			index++;
		}
	}
}
