#include "MaterialInspecterWidget.h"
#include <Core/SerializationStream.h>
#include <Widgets/QFloatField.h>
#include "../Texture/QTextureField.h"
#include "../Texture/QColorField.h"

MaterialInspecterWidget::MaterialInspecterWidget(QWidget* parent, LuxonEngine::SerializationStream* stream, std::string path)
	: QWidget(parent)
{
	ui.setupUi(this);
	auto guid = stream->GetGuid("uuid");
	auto material = GetAssetManager()->GetMaterial(guid);
	auto valueFields = material->GetValueFields();

	for(auto& [fieldName, valueData] : *valueFields) {
		if(fieldName[0] == '_') {
			continue; // Skip private fields
		}

		switch(valueData.size) {
			case sizeof(Int32) : 
			{
				auto floatfield = new QFloatField(ui.dataFields);
				floatfield->setLabelText(QString::fromStdString(fieldName));
				floatfield->setValue(material->GetValue<float>(fieldName, 0.0f));
				ui.dataFields->layout()->addWidget(floatfield);
				ui.dataFields->layout()->setAlignment(floatfield, Qt::AlignTop);
				break; 
			}
			case sizeof(Color):
			{
				auto colorField = new QColorField(ui.dataFields, fieldName);
				colorField->SetColor(material->GetValue<Color>(fieldName, Color()));
				ui.dataFields->layout()->addWidget(colorField);
				ui.dataFields->layout()->setAlignment(colorField, Qt::AlignTop);
				break;
			}
			default:
				// Handle other sizes if necessary
				break;
		}
	}

	auto textureFields = material->GetTextureFields();

	for(auto& [fieldName, textureData] : *textureFields) {
		if(fieldName[0] == '_') {
			continue; // Skip private fields
		}
		auto textureField = new QTextureField(ui.dataFields, fieldName);
		ui.dataFields->layout()->addWidget(textureField);
		ui.dataFields->layout()->setAlignment(textureField, Qt::AlignTop);
		textureField->SetTexture(textureData.texture);
	}
}

MaterialInspecterWidget::~MaterialInspecterWidget()
{}

