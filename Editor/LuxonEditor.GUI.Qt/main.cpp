#include "Windows/LuxonEditorWindow.h"
#include "Windows/SplashStartWidget.h"
#include <QtWidgets/QApplication>
#include <LuxonEditorAPI.h>

namespace QT = LuxonEditor::GUI::QT;

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    app.setOrganizationName("LuxonEngine");
    app.setApplicationName("LuxonEditor");

    QPalette dark;
    dark.setColor(QPalette::Window, QColor(53, 53, 53));
    dark.setColor(QPalette::WindowText, Qt::white);
    dark.setColor(QPalette::Base, QColor(25, 25, 25));
    dark.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    dark.setColor(QPalette::ToolTipBase, Qt::white);
    dark.setColor(QPalette::ToolTipText, Qt::white);
    dark.setColor(QPalette::Text, Qt::white);
    dark.setColor(QPalette::Button, QColor(53, 53, 53));
    dark.setColor(QPalette::ButtonText, Qt::white);
    dark.setColor(QPalette::BrightText, Qt::red);
    dark.setColor(QPalette::Link, QColor(42, 130, 218));
    dark.setColor(QPalette::Highlight, QColor(42, 130, 218));
    dark.setColor(QPalette::HighlightedText, Qt::black);

    app.setPalette(dark);

    app.setStyleSheet(R"(
        QMenuBar {
            background-color: #353535;
            color: white;
        }

        QMenuBar::item {
            background-color: #353535;
            color: white;
        }

        QMenuBar::item:selected {
            background-color: #454545;
        }

        QMenu {
            background-color: #353535;
            color: white;
        }

        QMenu::item:selected {
            background-color: #454545;
        }
    )");

    LuxonEditor::ApplicationConfig config{
        .projectPath = QCoreApplication::applicationDirPath().toStdString(),
        .graphicAPI = Graphic_API::DIRECTX_12,
    };
    std::string errorString;
    auto engineApp = CreateEngineApplication(config, errorString);

    QT::SplashStartWidget splashWindow;
    splashWindow.show();

    splashWindow.SetProgress(30, "Compiling Shaders...");
    engineApp->CompileShaders();
    splashWindow.SetProgress(60, "Loading Assets...");
    engineApp->LoadAssets();
    splashWindow.SetProgress(100, "Starting Editor...");

    QT::LuxonEditorWindow mainWindow;
    splashWindow.close();
    mainWindow.showMaximized();
    return app.exec();
}
