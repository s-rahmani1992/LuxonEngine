#include "Windows/LuxonEditorWindow.h"
#include <QtWidgets/QApplication>

namespace QT = LuxonEditor::GUI::QT;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QT::LuxonEditorWindow window;
    window.show();
    return app.exec();
}
