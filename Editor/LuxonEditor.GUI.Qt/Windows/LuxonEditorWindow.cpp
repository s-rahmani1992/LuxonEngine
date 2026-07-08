#include "LuxonEditorWindow.h"
#include <LuxonEditorAPI.h>
#include "Console/ConsoleLogWindow.h"
#include "AssetBrowser/AssetBrowserWindow.h"
#include "ShaderCreation/ShaderCreationWindow.h"
#include "InspecterWindow.h"

//TODO Right now, The ADS docking is copied into this project. use the git submodule in the future
#include "../ADS/src/DockManager.h"
#include "../ADS/src/DockWidget.h"
#include "../ADS/src/DockAreaWidget.h"
#include "../ADS/src/DockSplitter.h"


LuxonEditor::GUI::QT::LuxonEditorWindow::LuxonEditorWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    connect(ui.createShaderMenuItem, &QAction::triggered, [this]() {
        ShaderCreationWindow shaderWindow(this);
        shaderWindow.exec();
        });
    connect(ui.closeMenuItem, &QAction::triggered, [this]() {
        QApplication::quit();
        });

    ads::CDockManager::setConfigFlag(ads::CDockManager::DefaultDockAreaButtons, false);
    m_dockManager = new ads::CDockManager(ui.windowDocker);
    ui.windowDocker->layout()->addWidget(m_dockManager);

    auto consoleWindow = new ConsoleLogWindow(m_dockManager);
    auto* consoleDock = m_dockManager->createDockWidget("Console");
    consoleDock->setWidget(consoleWindow);

    auto assetBrowserWindow = new AssetBrowserWindow(QString::fromStdString(GetProjectPath() + "/Assets"), QString::fromStdString(GetProjectPath() + "/Assets"));
    auto* assetBrowserDock = m_dockManager->createDockWidget("Browser");
    assetBrowserDock->setWidget(assetBrowserWindow);

	auto inspecterWindow = new InspecterWindow(m_dockManager);
	auto* inspecterDock = m_dockManager->createDockWidget("Inspecter");
	inspecterDock->setWidget(inspecterWindow);

	auto* leftTopArea = m_dockManager->addDockWidget(ads::LeftDockWidgetArea, assetBrowserDock);

	auto* rightArea = m_dockManager->addDockWidget(ads::RightDockWidgetArea, inspecterDock);

    auto* splitter = leftTopArea->parentSplitter();

    int total = splitter->width();
    int left = total * 0.9;
    int right = total * 0.1;

    splitter->setSizes({ left, right });

    auto* leftBottomArea = m_dockManager->addDockWidget(ads::BottomDockWidgetArea, consoleDock, leftTopArea);

	splitter = leftBottomArea->parentSplitter();
	total = splitter->height();
    int top = total * 0.5;
    int bottom = total * 0.5;
	splitter->setSizes({ top, bottom });
}

LuxonEditor::GUI::QT::LuxonEditorWindow::~LuxonEditorWindow()
{}
