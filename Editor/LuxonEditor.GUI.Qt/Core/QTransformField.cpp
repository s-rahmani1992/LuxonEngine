#include "QTransformField.h"
#include <LuxonEditorAPI.h>
#include <qabstractitemview.h>

QTransformField::QTransformField(QWidget *parent, std::string fieldName, ref<LuxonEngine::Transform> transform)
	: QWidget(parent), m_transform(transform)
{
	ui.setupUi(this);
	ui.label->setText(QString::fromStdString(fieldName));

	auto& entities = LuxonEditor::EngineApplication::GetSceneManager()->GetEntityList();
	ui.selectBox->view()->setMinimumWidth(200);

	int index = 0;
	int transformIndex = 0;
	for (auto& entry : entities)
	{
		if(entry.uuid.is_nil())
			continue;

		ui.selectBox->addItem(QString::fromStdString(entry.entity->GetName()),
			QString::fromStdString(LuxonEditor::GuidGenerator::GUIDToString(entry.uuid)));
		ui.selectBox->setItemData(index, QString::fromStdString(entry.entity->GetName()), Qt::ToolTipRole);
		
		if(entry.entity->GetTransform() == m_transform)
		{
			transformIndex = index;
		}
		index++;
	}

	ui.selectBox->setCurrentIndex(transformIndex);

	connect(ui.selectBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx) {
		if (idx >= 0)
		{
			auto guid = LuxonEditor::GuidGenerator::GenerateGUIDFromString(
				ui.selectBox->itemData(idx).toString().toStdString());
			auto transform = LuxonEditor::EngineApplication::GetSceneManager()->GetTransformOfEntity(guid);
			if (transform != m_transform)
			{
				SetTransform(transform);
				emit ValueChanged(m_transform);
			}
		}
		});
}

QTransformField::~QTransformField()
{}

void QTransformField::SetTransform(ref<LuxonEngine::Transform> transform)
{
	m_transform = transform;
	auto guid = LuxonEditor::EngineApplication::GetSceneManager()->GetEntityGUIDFromTransform(transform);

	auto index = ui.selectBox->findData(QString::fromStdString(LuxonEditor::GuidGenerator::GUIDToString(guid)));

	if(index >= 0)
	{
		ui.selectBox->setCurrentIndex(index);
	}
}

