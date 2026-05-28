#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_LuxonEditorWindow.h"

namespace LuxonEditor::GUI::Qt {
    class LuxonEditorWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        LuxonEditorWindow(QWidget* parent = nullptr);
        ~LuxonEditorWindow();

    private:
        Ui::LuxonEditorWindowClass ui;
    };
}
