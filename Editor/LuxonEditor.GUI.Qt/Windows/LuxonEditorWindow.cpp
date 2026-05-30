#include "LuxonEditorWindow.h"
#include <EngineAPI.h>
#include "Console/ConsoleLogWindow.h"

//TODO Right now, The ADS docking is copied into this project. use the git submodule in the future
#include "../ADS/src/DockManager.h"
#include "../ADS/src/DockWidget.h"


LuxonEditor::GUI::QT::LuxonEditorWindow::LuxonEditorWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    connect(ui.logBtn, &QPushButton::clicked, this, [this]() {
        LuxonEngine::Logger::Log("log message");
        });
    connect(ui.warnBtn, &QPushButton::clicked, this, [this]() {
        LuxonEngine::Logger::LogWarning("warn message");
        });
    connect(ui.errorBtn, &QPushButton::clicked, this, [this]() {
        LuxonEngine::Logger::LogError("error message");
        });

    ads::CDockManager::setConfigFlag(ads::CDockManager::DefaultDockAreaButtons, false);
    m_dockManager = new ads::CDockManager(ui.windowDocker);
    ui.windowDocker->layout()->addWidget(m_dockManager);

    auto consoleWindow = new ConsoleLogWindow(m_dockManager);
    auto* consoleDock = m_dockManager->createDockWidget("Console");
    consoleDock->setWidget(consoleWindow);

    m_dockManager->addDockWidget(ads::CenterDockWidgetArea, consoleDock);
}

LuxonEditor::GUI::QT::LuxonEditorWindow::~LuxonEditorWindow()
{}
