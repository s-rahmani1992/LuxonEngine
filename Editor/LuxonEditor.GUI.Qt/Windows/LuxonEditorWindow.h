#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_LuxonEditorWindow.h"

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
        Ui::LuxonEditorWindowClass ui;
        ads::CDockManager* m_dockManager;
    };
}
