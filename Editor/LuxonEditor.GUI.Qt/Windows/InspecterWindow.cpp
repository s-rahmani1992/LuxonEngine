#include "InspecterWindow.h"
#include <LuxonEditorAPI.h>
#include "../ShaderCreation/ShaderInspecterWidget.h"
#include "../Mesh/Model3DInspecterWidget.h"
#include "../Texture/TextureInspecterWidget.h"
#include "../Material/MaterialInspecterWidget.h"
#include "../Scene/GameEntityInspecterWidget.h"
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

		if(selectedObject.starts_with("GameEntity:"))
		{
			std::string entityIdStr = selectedObject.substr(std::string("GameEntity:").length());
			auto entityId = GuidGenerator::GenerateGUIDFromString(entityIdStr);
			auto entity = EngineApplication::GetSceneManager()->GetEntityByUUID(entityId);
			if (entity)
			{
				ui.fileLabel->setText(QString::fromStdString(entity->GetName()));
				GameEntityInspecterWidget* entityWidget = new GameEntityInspecterWidget(ui.container, entity);
				m_currentWidget = entityWidget;
				m_currentWidget->show();
				ui.container->layout()->addWidget(m_currentWidget);
				return;
			}
		}

		// Check if selected object is a path with .hlsl extension
		std::filesystem::path selectedPath(selectedObject);

		ui.fileLabel->setText(QString::fromStdString(selectedPath.filename().string()));
		auto extention = selectedPath.extension().string();

		if (extention == ".hlsl" && std::filesystem::exists(selectedPath))
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

		else if(extention == ".fbx" && std::filesystem::exists(selectedPath))
		{
			// Create meta file path by appending .json
			m_metaPath = selectedObject + ".json";
			// Create SerializationStream from the meta file
			m_stream.LoadFromFile(m_metaPath);
			// Create Model3DInspecterWidget with root widget as parent
			Model3DInspecterWidget* modelWidget = new Model3DInspecterWidget(ui.container, &m_stream);
			
			connect(modelWidget, &Model3DInspecterWidget::PropertyUpdated, this, [this](LuxonEngine::SerializationStream* stream) {
				m_stream = *stream;
				std::string b = m_stream.ToString();
				m_stream.SaveToFile(m_metaPath);
				});
			m_currentWidget = modelWidget;
			m_currentWidget->show();
		}
		else if((extention == ".png" || extention == ".jpg" || extention == ".jpeg") && std::filesystem::exists(selectedPath))
		{
			// Create meta file path by appending .json
			m_metaPath = selectedObject + ".json";
			// Create SerializationStream from the meta file
			m_stream.LoadFromFile(m_metaPath);

			TextureInspecterWidget* textureWidget = new TextureInspecterWidget(ui.container, &m_stream, selectedObject);
			m_currentWidget = textureWidget;
			m_currentWidget->show();
		}
		else if(extention == ".lmat" && std::filesystem::exists(selectedPath))
		{
			// Create meta file path by appending .json
			m_metaPath = selectedObject + ".json";
			// Create SerializationStream from the meta file
			m_stream.LoadFromFile(m_metaPath);
			MaterialInspecterWidget* materialWidget = new MaterialInspecterWidget(ui.container, &m_stream, selectedObject);
			m_currentWidget = materialWidget;
			m_currentWidget->show();
		}
		else
		{
			m_currentWidget = new QLabel("No Inspecter Available", ui.container);
			m_currentWidget->show();
		}
		ui.container->layout()->addWidget(m_currentWidget);
	});
}

LuxonEditor::GUI::QT::InspecterWindow::~InspecterWindow()
{}

