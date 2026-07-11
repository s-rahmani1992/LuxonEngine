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

    private:
        template<class T, class... Args>
        auto AddWindow(QString name, Args&&... args);

        Ui::LuxonEditorWindowClass ui;
        ads::CDockManager* m_dockManager;
    };

    template<class T, class ...Args>
    inline auto LuxonEditorWindow::AddWindow(QString name, Args && ...args)
    {
        T* window = new T(std::forward<Args>(args)...);
        auto* dock = m_dockManager->createDockWidget(name);
        dock->setWidget(window);
        return dock;
    }
}
