#include "Windows/LuxonEditorWindow.h"
#include <QtWidgets/QApplication>
#include <LuxonEditorAPI.h>

namespace QT = LuxonEditor::GUI::QT;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    LuxonEditor::ApplicationConfig config{
        .projectPath = QCoreApplication::applicationDirPath().toStdString(),
        .graphicAPI = Graphic_API::DIRECTX_12,
    };
	std::string errorString;
	auto engineApp = CreateEngineApplication(config, errorString);

    QT::LuxonEditorWindow window;
    window.show();
    return app.exec();
}
