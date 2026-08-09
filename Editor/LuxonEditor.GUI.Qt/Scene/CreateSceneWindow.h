#pragma once

#include <QDialog>
#include <filesystem>

namespace fs = std::filesystem;

class QTextField;

class CreateSceneWindow : public QDialog
{
	Q_OBJECT

public:
	CreateSceneWindow(QWidget* parent = nullptr);
	~CreateSceneWindow();

private slots:
	void OnBrowseButtonClicked();
	void OnCreateButtonClicked();
	void OnNameValidationChanged(bool isValid);

private:
	void RefreshCreateButton();

	QTextField* m_nameField = nullptr;
	class QLabel* m_pathLabel = nullptr;
	class QPushButton* m_createButton = nullptr;
	class QCheckBox* m_openAfterCreate = nullptr;

	fs::path m_folderPath;
	bool m_nameIsValid = true;
};