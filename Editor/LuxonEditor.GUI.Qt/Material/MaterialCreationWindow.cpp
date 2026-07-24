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

		auto shaderEntries = m_shaderRegistry->GetAllShaderEntries();

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

			dlg.setDirectory(QString::fromStdString(GetProjectPath() + m_folderPath.string()));
			//dlg.setDirectory(QString::fromStdString(m_folderPath.string()));
			if (dlg.exec() == QDialog::Accepted) {
				m_folderPath = dlg.selectedFiles().first().toStdString();
				ui.pathLabel->setText(QString::fromStdString((m_folderPath / (ui.nameField->InputText()->text().toStdString() + ".lmat")).string()));
			}
			});

		if (ui.programBox->count() > 0)
			OnProgramIndexChanged(0);
	}

	MaterialCreationWindow::~MaterialCreationWindow()
	{
	}

	void MaterialCreationWindow::OnNameValidationChanged(bool isValid)
	{
		m_nameIsValid = isValid;
		ui.pathLabel->setText(QString::fromStdString((m_folderPath / (ui.nameField->InputText()->text().toStdString() + ".lmat")).string()));
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
			ui.errorLabel->setText(QString::fromStdString(entry->compileError));
		}
		else
		{
			ui.errorLabel->clear();
			if (entry)
				m_currentProgram = entry->program;
		}

		RefreshCreateButton();
	}

	void MaterialCreationWindow::RefreshCreateButton()
	{
		ui.createButton->setEnabled(m_nameIsValid && m_currentProgram != nullptr);
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

		std::string name = ui.nameField->InputText()->text().toStdString();
		fs::path filePath = m_folderPath / (name + ".lmat");

		LuxonEngine::SerializationStream stream;
		LuxonEditor::MaterialImporter::SerializeMaterial(material, stream);
		stream.SaveToFile(filePath.string());

		LuxonEngine::SerializationStream metaStream = LuxonEditor::MaterialImporter::CreateDefaultMetaData();
		metaStream.SaveToFile((filePath.string() + ".json").c_str());
		
		GetAssetManager()->AddMaterial(metaStream.GetGuid("uuid"), material);
		//GetAssetManager()->ImportAsset(filePath);

		accept();
	}

}