#include "InspecterWindow.h"
#include <LuxonEditorAPI.h>
#include "../ShaderCreation/ShaderInspecterWidget.h"
#include <filesystem>
#include <Core/SerializationStream.h>

LuxonEditor::GUI::QT::InspecterWindow::InspecterWindow(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	GetSelectionManager()->RegisterSelectionChangedCallback([this](const std::string& selectedObject) {
		// Clean up previous shader widget
		if (m_currentWidget != nullptr)
		{
			m_currentWidget->deleteLater();
			m_currentWidget = nullptr;
		}
		// Check if selected object is a path with .hlsl extension
		std::filesystem::path selectedPath(selectedObject);

		ui.fileLabel->setText(QString::fromStdString(selectedPath.filename().string()));

		if (selectedPath.extension().string() == ".hlsl" && std::filesystem::exists(selectedPath))
		{
			// Create meta file path by appending .json
			std::string metaFilePath = selectedObject + ".json";
			
			// Create SerializationStream from the meta file
			LuxonEngine::SerializationStream serializationStream;
			serializationStream.LoadFromFile(metaFilePath);
			
			// Create ShaderInspecterWidget with root widget as parent
			LuxonEngine::SerializationStream data = serializationStream.Object("data");
			m_currentWidget = new ShaderInspecterWidget(ui.container, &data);
			m_currentWidget->show();
		}
	});
}

LuxonEditor::GUI::QT::InspecterWindow::~InspecterWindow()
{}

