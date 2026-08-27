#include "QMaterialField.h"
#include <LuxonEditorAPI.h>
#include <qabstractitemview.h>

QMaterialField::QMaterialField(QWidget* parent, std::string fieldName,
	LuxonEngine::Rendering::ShaderProgramType programType)
	: QWidget(parent), m_programType(programType)
{
	ui.setupUi(this);
	ui.label->setText(QString::fromStdString(fieldName));

	auto materials = GetAssetManager()->GetAllMaterialEntries();
	ui.selectBox->view()->setMinimumWidth(200);

	int index = 0;
	for (auto& entry : materials)
	{
		if (entry->asset && entry->asset->GetProgram() &&
			entry->asset->GetProgram()->GetType() == m_programType)
		{
			ui.selectBox->addItem(QString::fromStdString(entry->name),
				QString::fromStdString(LuxonEditor::GuidGenerator::GUIDToString(entry->guid)));
			ui.selectBox->setItemData(index, QString::fromStdString(entry->name), Qt::ToolTipRole);
			index++;
		}
	}

	ui.selectBox->setCurrentIndex(-1);

	connect(ui.selectBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx) {
		if (idx >= 0)
		{
			auto guid = LuxonEditor::GuidGenerator::GenerateGUIDFromString(
				ui.selectBox->itemData(idx).toString().toStdString());
			auto material = GetAssetManager()->GetMaterial(guid);
			if (material != m_material)
			{
				SetMaterial(material);
			}
		}
		});
}

QMaterialField::QMaterialField(QWidget* parent, std::string fieldName)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.label->setText(QString::fromStdString(fieldName));

	auto materials = GetAssetManager()->GetAllMaterialEntries();
	ui.selectBox->view()->setMinimumWidth(200);

	int index = 0;
	for (auto& entry : materials)
	{
		if (entry->asset && entry->asset->GetProgram())
		{
			ui.selectBox->addItem(QString::fromStdString(entry->name),
				QString::fromStdString(LuxonEditor::GuidGenerator::GUIDToString(entry->guid)));
			ui.selectBox->setItemData(index, QString::fromStdString(entry->name), Qt::ToolTipRole);
			index++;
		}
	}

	ui.selectBox->setCurrentIndex(-1);

	connect(ui.selectBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx) {
		if (idx >= 0)
		{
			auto guid = LuxonEditor::GuidGenerator::GenerateGUIDFromString(
				ui.selectBox->itemData(idx).toString().toStdString());
			auto material = GetAssetManager()->GetMaterial(guid);
			if (material != m_material)
			{
				SetMaterial(material);
			}
		}
		});
}

QMaterialField::~QMaterialField()
{
}

void QMaterialField::SetMaterial(ref<LuxonEngine::Rendering::Material> material)
{
	if (material == m_material)
		return;

	m_material = material;

	auto entry = GetAssetManager()->GetMaterialEntry(material);

	if(entry == nullptr)
		return;

	int index = ui.selectBox->findData(
		QString::fromStdString(LuxonEditor::GuidGenerator::GUIDToString(entry->guid)));
	if (index >= 0)
	{
		ui.selectBox->setCurrentIndex(index);
	}

	emit ValueChanged(material);
}