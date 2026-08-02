#include "QMeshField.h"
#include <LuxonEditorAPI.h>
#include <qabstractitemview.h>

QMeshField::QMeshField(QWidget* parent, std::string fieldName)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.label->setText(QString::fromStdString(fieldName));

	auto meshes = GetAssetManager()->GetAllMeshEntries();
	ui.selectBox->view()->setMinimumWidth(200);

	int index = 0;
	for (auto& mesh : meshes)
	{
		ui.selectBox->addItem(QString::fromStdString(mesh->name), QString::fromStdString(LuxonEditor::GuidGenerator::GUIDToString(mesh->guid)));
		ui.selectBox->setItemData(index, QString::fromStdString(mesh->name), Qt::ToolTipRole);
		index++;
	}

	ui.selectBox->setCurrentIndex(-1);

	connect(ui.selectBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
		if (index >= 0)
		{
			auto guid = LuxonEditor::GuidGenerator::GenerateGUIDFromString(ui.selectBox->itemData(index).toString().toStdString());
			auto mesh = GetAssetManager()->GetMesh(guid);
			if (mesh != m_mesh)
			{
				SetMesh(mesh);
			}
		}
		});
}

QMeshField::~QMeshField()
{
}

void QMeshField::SetMesh(ref<LuxonEngine::Mesh> mesh)
{
	if (mesh == m_mesh)
		return;

	m_mesh = mesh;

	auto entry = GetAssetManager()->GetMeshEntry(mesh);

	int index = ui.selectBox->findData(QString::fromStdString(LuxonEditor::GuidGenerator::GUIDToString(entry->guid)));
	if (index >= 0)
	{
		ui.selectBox->setCurrentIndex(index);
	}

	emit ValueChanged(mesh);
}