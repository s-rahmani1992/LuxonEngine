#include "Model3DInspecterWidget.h"
#include <Core/SerializationStream.h>
#include "MeshItem.h"
#include <LuxonEditorAPI.h>
#include <Widgets/QVector3Field.h>

LuxonEditor::GUI::QT::Model3DInspecterWidget::Model3DInspecterWidget(QWidget *parent, LuxonEngine::SerializationStream* stream)
	: QWidget(parent)
{
	ui.setupUi(this);

	LuxonEditor::ModelImportProperties properties;
	auto transformStream = stream->Object("transform");
	AssimpModel3DImporter::FillPropertiesFromStream(&transformStream, properties);

	m_positionField = new QVector3Field(ui.transformPanel);
	ui.transformPanel->layout()->addWidget(m_positionField);
	ui.transformPanel->layout()->setAlignment(m_positionField, Qt::AlignTop);
	m_positionField->setLabelText("Position");
	m_positionField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_positionField->setValue(properties.position);

	m_rotationAngleField = new QFloatField(ui.transformPanel);
	ui.transformPanel->layout()->addWidget(m_rotationAngleField);
	ui.transformPanel->layout()->setAlignment(m_rotationAngleField, Qt::AlignTop);
	m_rotationAngleField->setLabelText("Rotation Angle");
	m_rotationAngleField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_rotationAngleField->setValue(properties.angleDeg);

	m_rotationAxisField = new QVector3Field(ui.transformPanel);
	ui.transformPanel->layout()->addWidget(m_rotationAxisField);
	ui.transformPanel->layout()->setAlignment(m_rotationAxisField, Qt::AlignTop);
	m_rotationAxisField->setLabelText("Rotation Axis");
	m_rotationAxisField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_rotationAxisField->setValue(properties.axis);

	m_scaleField = new QVector3Field(ui.transformPanel);
	ui.transformPanel->layout()->addWidget(m_scaleField);
	ui.transformPanel->layout()->setAlignment(m_scaleField, Qt::AlignTop);
	m_scaleField->setLabelText("Scale");
	m_scaleField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_scaleField->setValue(properties.scale);

	ui.meshList->setStyleSheet("#meshList { background-color: #2b2b5b; }");

	m_meshes = stream->Array("meshes");

	for (auto& meshStream : m_meshes)
	{
		MeshItem* item = new MeshItem(ui.meshList, &meshStream);
		ui.meshList->layout()->addWidget(item);
		ui.meshList->layout()->setAlignment(item, Qt::AlignTop);
	}

	connect(ui.importButton, &QPushButton::clicked, this, [this, stream]() {
		ModelImportProperties properties;
		properties.position = m_positionField->value();
		properties.angleDeg = m_rotationAngleField->value();
		properties.axis = m_rotationAxisField->value();
		properties.scale = m_scaleField->value();
		SerializationStream propStream;
		AssimpModel3DImporter::SerializePropertiesToStream(properties, &propStream);
		emit PropertyUpdated(&propStream);
		});
}

LuxonEditor::GUI::QT::Model3DInspecterWidget::~Model3DInspecterWidget()
{}

