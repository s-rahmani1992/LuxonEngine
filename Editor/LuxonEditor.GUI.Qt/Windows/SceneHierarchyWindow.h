#pragma once
#include <QWidget>
#include "ui_SceneHierarchyWindow.h"

namespace LuxonEditor {
	class EngineSceneManager;
}

namespace LuxonEditor::GUI::QT {
	class GameEntityModel;
	class GameEntityItemDelegate;

	class SceneHierarchyWindow : public QWidget
	{
		Q_OBJECT

	public:
		explicit SceneHierarchyWindow(QWidget* parent = nullptr);
		~SceneHierarchyWindow();

	protected:
		bool eventFilter(QObject* watched, QEvent* event) override;

	private slots:
		void OnAddEntityRequested();

	private:
		void OnDeleteRequested(const QModelIndex& index);

		Ui::SceneHierarchyWindowClass ui;
		LuxonEditor::GUI::QT::GameEntityModel* m_entityModel = nullptr;
		LuxonEditor::GUI::QT::GameEntityItemDelegate* m_entityDelegate = nullptr;

		QListWidgetItem* m_lightItem = nullptr;
		QListWidgetItem* m_cameraItem = nullptr;
		QListWidgetItem* m_rtItem = nullptr;
		QListWidgetItem* m_behaviourItem = nullptr;
	};
}