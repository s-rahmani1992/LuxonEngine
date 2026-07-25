#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_LuxonEditorWindow.h"
#include "../ADS/src/DockManager.h"
#include "../ADS/src/DockWidget.h"
#include "../ADS/src/DockAreaWidget.h"

namespace ads {
    class CDockManager;
}

namespace LuxonEditor::GUI::QT {
    class LuxonEditorWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        LuxonEditorWindow(QWidget* parent = nullptr);
        ~LuxonEditorWindow();

    protected:
        void closeEvent(QCloseEvent* event) override;

    private:
        void setupDefaultDockLayout();
        void restoreDockState();
		void SetupMenuItems(ads::CDockWidget* dockWidget, QAction* menuItem);
        void OnMenuItemTriggered(ads::CDockWidget* dockWidget, QAction* menuItem);
		void UpdateDockWidgetMenuItemState(ads::CDockWidget* dockWidget, QAction* menuItem);
        
        template<class T, class... Args>
        auto AddWindow(QString name, Args&&... args);

        Ui::LuxonEditorWindowClass ui;
        ads::CDockManager* m_dockManager = nullptr;
        ads::CDockWidget* m_consoleDock = nullptr;
        ads::CDockWidget* m_assetBrowserDock = nullptr;
        ads::CDockWidget* m_inspecterDock = nullptr;
        ads::CDockWidget* m_sceneEditorDock = nullptr;
        ads::CDockWidget* m_sceneHierarchyDock = nullptr;
    };

    template<class T, class ...Args>
    inline auto LuxonEditorWindow::AddWindow(QString name, Args && ...args)
    {
        T* window = new T(std::forward<Args>(args)...);
        auto* dock = m_dockManager->createDockWidget(name);
        dock->setWidget(window);
        dock->setMinimumSizeHintMode(ads::CDockWidget::MinimumSizeHintFromContentMinimumSize);
        return dock;
    }
}