#include "MaterialCreationWindow.h"

#include <LuxonEditorAPI.h>
#include <Core/EngineApplication.h>
#include <Core/EngineShaderRegistry.h>
#include <Core/MaterialImporter.h>
#include <Core/GuidUtilities.h>
#include <Core/SerializationStream.h>
#include <Rendering/MaterialFactory.h>
#include <boost/uuid/uuid.hpp>
#include <qfiledialog.h>
#include <qfilesystemmodel.h>
#include <qabstractitemview.h>

namespace LuxonEditor::GUI::QT {

	MaterialCreationWindow::MaterialCreationWindow(QWidget* parent)
		: QDialog(parent), m_folderPath(GetProjectPath() + "/Assets/"), m_shaderRegistry(EngineApplication::GetShaderRegistery()), m_currentProgram(nullptr)
	{
		ui.setupUi(this);
		ui.errorScroll->setVisible(false);
		ui.errorScroll->setWidget(ui.errorLabel);
		ui.errorScroll->setStyleSheet(ui.errorScroll->styleSheet() + "#errorScroll {border: 2px solid darkred; border-radius: 4px; }");
		ui.pathPanel->layout()->setAlignment(ui.nameField, Qt::AlignmentFlag::AlignLeft);
		ui.pathPanel->layout()->setAlignment(ui.browseButton, Qt::AlignmentFlag::AlignLeft);
		ui.errorLabel->setStyleSheet(ui.errorLabel->styleSheet() + "padding: 6px;");
		layout()->setAlignment(ui.shaderTypeField, Qt::AlignmentFlag::AlignTop);
		layout()->setAlignment(ui.errorScroll, Qt::AlignmentFlag::AlignTop);
		layout()->setAlignment(ui.createButton, Qt::AlignmentFlag::AlignBottom | Qt::AlignmentFlag::AlignHCenter);
		layout()->setAlignment(ui.pathPanel, Qt::AlignmentFlag::AlignTop);
		static_cast<QHBoxLayout*>(ui.pathPanel->layout())->addStretch(1);

		static_cast<QVBoxLayout*>(layout())->insertStretch(layout()->indexOf(ui.createButton), 1);		auto shaderEntries = m_shaderRegistry->GetAllShaderEntries();

		for (auto* entry : shaderEntries)
		{
			ui.programBox->addItem(
				QString::fromStdString(entry->name),
				QString::fromStdString(LuxonEditor::GuidGenerator::GUIDToString(entry->guid))
			);
		}

		ui.nameField->RegisterValidationFunction([this](const QString& text) -> bool {
			if (text.isEmpty())
				return false;
			fs::path filePath = m_folderPath / (text.toStdString() + ".lmat");
			return !fs::exists(filePath);
			});

		connect(ui.nameField, &QTextField::ValueChanged,
			this, &MaterialCreationWindow::OnNameValidationChanged);

		connect(ui.programBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &MaterialCreationWindow::OnProgramIndexChanged);

		connect(ui.createButton, &QPushButton::clicked,
			this, &MaterialCreationWindow::OnCreateButtonClicked);

		connect(ui.browseButton, &QPushButton::clicked, this, [this]() {
			QFileDialog dlg;
			dlg.setFileMode(QFileDialog::Directory);
			dlg.setOption(QFileDialog::ShowDirsOnly, true);
			dlg.setOption(QFileDialog::DontUseNativeDialog, true);

			QFileSystemModel* model = new QFileSystemModel(&dlg);
			QModelIndex rootIndex = model->setRootPath(QString::fromStdString(GetProjectPath() + "/Assets/"));

			auto views = dlg.findChildren<QAbstractItemView*>();
			for (QAbstractItemView* view : views) {
				view->setRootIndex(rootIndex);
			}

			dlg.setDirectory(QString::fromStdString(m_folderPath.string()));
			if (dlg.exec() == QDialog::Accepted) {
				m_folderPath = dlg.selectedFiles().first().toStdString();
				ui.nameField->Validate();
				ui.pathLabel->setText(QString::fromStdString((m_folderPath / (ui.nameField->InputText()->text().toStdString() + ".lmat")).lexically_normal().string()));
			}
			});

		int index = 0;
		std::string name = "NewMaterial_0";

		while (true)
		{
			fs::path filePath = m_folderPath / (name + ".lmat");
			if (!fs::exists(filePath))
				break;
			name = "NewMaterial_" + std::to_string(index++);
		}

		ui.nameField->InputText()->setText(QString::fromStdString(name));
		ui.pathLabel->setText(QString::fromStdString((m_folderPath / (ui.nameField->InputText()->text().toStdString() + ".lmat")).lexically_normal().string()));
		ui.createButton->setEnabled(false);
	}

	MaterialCreationWindow::~MaterialCreationWindow()
	{
	}

	void MaterialCreationWindow::OnNameValidationChanged(bool isValid)
	{
		m_nameIsValid = isValid;
		ui.pathLabel->setText(QString::fromStdString((m_folderPath / (ui.nameField->InputText()->text().toStdString() + ".lmat")).lexically_normal().string()));
		RefreshCreateButton();
	}

	void MaterialCreationWindow::OnProgramIndexChanged(int index)
	{
		m_currentProgram = nullptr;

		if (index < 0)
			return;

		m_currentGuid = LuxonEditor::GuidGenerator::GenerateGUIDFromString(
			ui.programBox->itemData(index).toString().toStdString()
		);

		auto* entry = m_shaderRegistry->GetShaderEntry(m_currentGuid);

		if (entry && !entry->compileError.empty())
		{
			ui.errorScroll->setVisible(true);
			ui.errorLabel->setText(QString::fromStdString(entry->compileError));
		}
		else
		{
			ui.errorScroll->setVisible(false);
			ui.errorLabel->clear();
			if (entry)
				m_currentProgram = entry->program;
		}

		RefreshCreateButton();
	}

	void MaterialCreationWindow::RefreshCreateButton()
	{
		int h = ui.programBox->currentIndex();
		ui.createButton->setEnabled(ui.programBox->currentIndex() >= 0 && m_nameIsValid && m_currentProgram != nullptr);
	}

	void MaterialCreationWindow::OnCreateButtonClicked()
	{
		if (!m_currentProgram)
			return;

		auto* gpuDevice = LuxonEditor::EngineApplication::GetGPUApplication();
		auto materialFactory = gpuDevice->CreateMaterialFactory();

		auto program = std::shared_ptr<LuxonEngine::Rendering::ShaderProgram>(m_currentProgram);
		auto material = materialFactory->CreateMaterial(program);

		if (!material)
			return;

		material->SetProgramGuid(m_currentGuid);
		std::string name = ui.nameField->InputText()->text().toStdString();
		fs::path filePath = m_folderPath / (name + ".lmat");

		LuxonEngine::SerializationStream stream;
		LuxonEditor::MaterialImporter::SerializeMaterial(material, stream);
		stream.SaveToFile(filePath.string());

		LuxonEngine::SerializationStream metaStream = LuxonEditor::MaterialImporter::CreateDefaultMetaData();
		metaStream.SaveToFile((filePath.string() + ".json").c_str());
		
		GetAssetManager()->AddMaterial(metaStream.GetGuid("uuid"), name, material);

		accept();
	}

}