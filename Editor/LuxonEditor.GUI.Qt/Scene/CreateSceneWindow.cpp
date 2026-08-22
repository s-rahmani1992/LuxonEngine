#include "CreateSceneWindow.h"

#include <Core/EngineApplication.h>
#include <Core/EngineSceneManager.h>
#include <Widgets/QTextField.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QAbstractItemView>

CreateSceneWindow::CreateSceneWindow(QWidget* parent)
	: QDialog(parent), m_folderPath(LuxonEditor::EngineApplication::GetProjectPath() + "/Assets/")
{
	setWindowTitle("Create Scene");
	setMinimumSize(400, 200);
	// --- name row ---
	auto* nameLayout = new QHBoxLayout();
	m_nameField = new QTextField(this);
	m_nameField->setLabelText("Name");

	auto* browseButton = new QPushButton("Browse...", this);

	nameLayout->addWidget(m_nameField);
	nameLayout->addWidget(browseButton, 0, Qt::AlignLeft);
	nameLayout->addStretch(1);

	// --- path label ---
	m_pathLabel = new QLabel(this);
	m_pathLabel->setWordWrap(true);

	// --- open-after-create checkbox ---
	m_openAfterCreate = new QCheckBox("Open scene after creation", this);
	m_openAfterCreate->setChecked(true);

	// --- create button ---
	m_createButton = new QPushButton("Create", this);
	m_createButton->setEnabled(true);

	// --- root layout ---
	auto* rootLayout = new QVBoxLayout(this);
	rootLayout->addLayout(nameLayout);
	rootLayout->addWidget(m_pathLabel);
	rootLayout->addWidget(m_openAfterCreate);
	rootLayout->addStretch(1);
	rootLayout->addWidget(m_createButton, 0, Qt::AlignHCenter | Qt::AlignBottom);
	setLayout(rootLayout);

	// --- validation ---
	m_nameField->RegisterValidationFunction([this](const QString& text) -> bool {
		if (text.isEmpty())
			return false;
		fs::path filePath = m_folderPath / (text.toStdString() + ".lscene");
		return !fs::exists(filePath);
		});

	// --- pick a unique default name ---
	int index = 0;
	std::string name = "NewScene_0";
	while (true)
	{
		fs::path candidate = m_folderPath / (name + ".lscene");
		if (!fs::exists(candidate))
			break;
		name = "NewScene_" + std::to_string(index++);
	}
	m_nameField->InputText()->setText(QString::fromStdString(name));
	m_pathLabel->setText(QString::fromStdString((m_folderPath / (name + ".lscene")).lexically_normal().string()));

	// --- connections ---
	connect(m_nameField, &QTextField::ValueChanged,
		this, &CreateSceneWindow::OnNameValidationChanged);

	connect(browseButton, &QPushButton::clicked,
		this, &CreateSceneWindow::OnBrowseButtonClicked);

	connect(m_createButton, &QPushButton::clicked,
		this, &CreateSceneWindow::OnCreateButtonClicked);
}

CreateSceneWindow::~CreateSceneWindow()
{
}

void CreateSceneWindow::OnNameValidationChanged(bool isValid)
{
	m_nameIsValid = isValid;
	m_pathLabel->setText(QString::fromStdString(
		(m_folderPath / (m_nameField->InputText()->text().toStdString() + ".lscene")).lexically_normal().string()
	));
	RefreshCreateButton();
}

void CreateSceneWindow::OnBrowseButtonClicked()
{
	QFileDialog dlg;
	dlg.setFileMode(QFileDialog::Directory);
	dlg.setOption(QFileDialog::ShowDirsOnly, true);
	dlg.setOption(QFileDialog::DontUseNativeDialog, true);

	QFileSystemModel* model = new QFileSystemModel(&dlg);
	QModelIndex rootIndex = model->setRootPath(
		QString::fromStdString(LuxonEditor::EngineApplication::GetProjectPath() + "/Assets/")
	);

	const auto views = dlg.findChildren<QAbstractItemView*>();
	for (QAbstractItemView* view : views)
		view->setRootIndex(rootIndex);

	dlg.setDirectory(QString::fromStdString(m_folderPath.string()));

	if (dlg.exec() == QDialog::Accepted)
	{
		m_folderPath = dlg.selectedFiles().first().toStdString();
		// re-run validation against the new folder
		m_nameField->Validate();
		m_pathLabel->setText(QString::fromStdString(
			(m_folderPath / (m_nameField->InputText()->text().toStdString() + ".lscene")).lexically_normal().string()
		));
	}
}

void CreateSceneWindow::OnCreateButtonClicked()
{
	if (!m_nameIsValid)
		return;

	std::string name = m_nameField->InputText()->text().toStdString();
	fs::path absolutePath = m_folderPath / (name + ".lscene");

	fs::path projectRoot = LuxonEditor::EngineApplication::GetProjectPath();
	std::string relativePath = fs::relative(absolutePath, projectRoot).string();

	auto* sceneManager = LuxonEditor::EngineApplication::GetSceneManager();
	sceneManager->CreateScene(name, relativePath, m_openAfterCreate->isChecked());

	accept();
}

void CreateSceneWindow::RefreshCreateButton()
{
	m_createButton->setEnabled(m_nameIsValid);
}