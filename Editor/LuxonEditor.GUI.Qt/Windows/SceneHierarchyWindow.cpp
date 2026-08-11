#include "SceneHierarchyWindow.h"
#include "../Scene/GameEntityModel.h"
#include "../Scene/GameEntityItemDelegate.h"
#include <Core/EngineSceneManager.h>
#include <Core/GameEntity.h>
#include <Core/Transform.h>
#include <QKeyEvent>
#include <QMessageBox>
#include <LuxonEditorAPI.h>

namespace LuxonEditor::GUI::QT {
	SceneHierarchyWindow::SceneHierarchyWindow(QWidget* parent)
		: QWidget(parent)
	{
		ui.setupUi(this);
		ui.otherComponents->setStyleSheet(R"(
			QListWidget::item:selected {
				background-color: #2a82da;
			}
		)");
		ui.otherComponents->setFocusPolicy(Qt::NoFocus);
		m_lightItem = new QListWidgetItem("Lights", ui.otherComponents);
		m_lightItem->setFlags(m_lightItem->flags() | Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		m_cameraItem = new QListWidgetItem("Camera", ui.otherComponents);
		m_cameraItem->setFlags(m_cameraItem->flags() | Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		m_rtItem = new QListWidgetItem("Ray Tracing", ui.otherComponents);
		m_rtItem->setFlags(m_rtItem->flags() | Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		m_entityModel = new LuxonEditor::GUI::QT::GameEntityModel(EngineApplication::GetSceneManager(), this);
		m_entityDelegate = new LuxonEditor::GUI::QT::GameEntityItemDelegate(this);

		ui.gameEntityList->setModel(m_entityModel);
		ui.gameEntityList->setItemDelegate(m_entityDelegate);

		// F2 triggers inline rename
		ui.gameEntityList->setEditTriggers(QAbstractItemView::EditKeyPressed);

		// Install event filter to intercept Delete key
		ui.gameEntityList->installEventFilter(this);

		connect(ui.gameEntityList, &QListView::clicked, this, [this](const QModelIndex& idx) {
			auto data = idx.data(LuxonEditor::GUI::QT::GameEntityModel::UUIDRole).toString();
			GetSelectionManager()->SetSelectedObject("GameEntity:" + data.toStdString());
			});

		connect(ui.otherComponents, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
			if (item == m_lightItem)
			{
				GetSelectionManager()->SetSelectedObject(std::string("Light"));
			}
			else if (item == m_cameraItem)
			{
				GetSelectionManager()->SetSelectedObject(std::string("Camera"));
			}
			else if (item == m_rtItem)
			{
				GetSelectionManager()->SetSelectedObject(std::string("RayTracing"));
			}
			});

		connect(ui.addEntityButton, &QPushButton::clicked, this, &SceneHierarchyWindow::OnAddEntityRequested);
	}

	SceneHierarchyWindow::~SceneHierarchyWindow()
	{
	}

	bool SceneHierarchyWindow::eventFilter(QObject* watched, QEvent* event)
	{
		if (watched == ui.gameEntityList && event->type() == QEvent::KeyPress)
		{
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent->key() == Qt::Key_Delete)
			{
				const QModelIndex index = ui.gameEntityList->currentIndex();
				if (index.isValid())
				{
					OnDeleteRequested(index);
					return true;
				}
			}
		}
		return QWidget::eventFilter(watched, event);
	}

	void SceneHierarchyWindow::OnAddEntityRequested()
	{
		auto transform = std::make_shared<LuxonEngine::Transform>();
		auto entity = std::make_shared<LuxonEngine::GameEntity>(transform, nullptr, nullptr);
		entity->SetName("new entity");
		EngineApplication::GetSceneManager()->AddEntity(entity);
	}

	void SceneHierarchyWindow::OnDeleteRequested(const QModelIndex& index)
	{
		const QString name = index.data(Qt::DisplayRole).toString();

		QMessageBox::StandardButton result = QMessageBox::question(
			this,
			tr("Delete Entity"),
			tr("Are you sure you want to delete entity \"%1\"?").arg(name),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::No);

		if (result == QMessageBox::Yes)
		{
			auto uuid = GuidGenerator::GenerateGUIDFromString(index.data(LuxonEditor::GUI::QT::GameEntityModel::UUIDRole).toString().toStdString());
			EngineApplication::GetSceneManager()->RemoveEntity(uuid);
		}
	}
}