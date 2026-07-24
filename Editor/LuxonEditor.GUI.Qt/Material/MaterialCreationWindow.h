#pragma once

#include <QDialog>
#include <filesystem>
#include <boost/uuid/uuid.hpp>
#include "ui_MaterialCreationWindow.h"

namespace LuxonEngine::Rendering {
	class ShaderProgram;
}

namespace LuxonEditor {
	class EngineShaderRegistry;
}

namespace fs = std::filesystem;

namespace LuxonEditor::GUI::QT {
	class MaterialCreationWindow : public QDialog
	{
		Q_OBJECT

	public:
		MaterialCreationWindow(QWidget* parent = nullptr);
		~MaterialCreationWindow();

	private slots:
		void OnProgramIndexChanged(int index);
		void OnCreateButtonClicked();
		void OnNameValidationChanged(bool isValid);

	private:
		void RefreshCreateButton();

		Ui::MaterialCreationWindowClass ui;
		fs::path m_folderPath;
		bool m_nameIsValid = false;

		LuxonEditor::EngineShaderRegistry* m_shaderRegistry;
		boost::uuids::uuid m_currentGuid;
		LuxonEngine::Rendering::ShaderProgram* m_currentProgram = nullptr;
	};
}