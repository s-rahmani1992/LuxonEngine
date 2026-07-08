#include "InspecterWindow.h"
#include <LuxonEditorAPI.h>
#include "../ShaderCreation/ShaderInspecterWidget.h"
#include <filesystem>

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
			m_metaPath = selectedObject + ".json";
			
			// Create SerializationStream from the meta file
			m_stream.LoadFromFile(m_metaPath);
			
			// Create ShaderInspecterWidget with root widget as parent
			m_dataStream = m_stream.Object("data");
			ShaderInspecterWidget* shaderWidget = new ShaderInspecterWidget(ui.container, &m_dataStream);
			connect(shaderWidget, &ShaderInspecterWidget::PropertyUpdates, this, [this](LuxonEngine::SerializationStream* stream) {
				m_stream.SetObject("data", *stream);
				std::string b = m_stream.ToString();
				m_stream.SaveToFile(m_metaPath);
				});
			m_currentWidget = shaderWidget;
			m_currentWidget->show();
		}
	});
}

LuxonEditor::GUI::QT::InspecterWindow::~InspecterWindow()
{}

