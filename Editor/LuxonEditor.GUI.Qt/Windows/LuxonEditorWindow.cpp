#include "LuxonEditorWindow.h"
#include <LuxonEditorAPI.h>
#include "Console/ConsoleLogWindow.h"
#include "AssetBrowser/AssetBrowserWindow.h"
#include "ShaderCreation/ShaderCreationWindow.h"
#include "Material/MaterialCreationWindow.h"
#include "InspecterWindow.h"
#include "SceneEditorWindow.h"

//TODO Right now, The ADS docking is copied into this project. use the git submodule in the future
#include "../ADS/src/DockSplitter.h"


LuxonEditor::GUI::QT::LuxonEditorWindow::LuxonEditorWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    connect(ui.createShaderMenuItem, &QAction::triggered, [this]() {
        ShaderCreationWindow shaderWindow(this);
        shaderWindow.exec();
        });
    connect(ui.createMaterialMenuItem, &QAction::triggered, [this]() {
        MaterialCreationWindow materialWindow(this);
        materialWindow.exec();
		});
    connect(ui.closeMenuItem, &QAction::triggered, [this]() {
        QApplication::quit();
        });

    ads::CDockManager::setConfigFlag(ads::CDockManager::DefaultDockAreaButtons, false);
    m_dockManager = new ads::CDockManager(ui.windowDocker);
    ui.windowDocker->layout()->addWidget(m_dockManager);

    auto* consoleDock = AddWindow<ConsoleLogWindow>("Console", m_dockManager);
    auto* assetBrowserDock = AddWindow<AssetBrowserWindow>("Browser", QString::fromStdString(GetProjectPath() + "/Assets"), QString::fromStdString(GetProjectPath() + "/Assets"));
    auto* inspecterDock = AddWindow<InspecterWindow>("Inspecter", m_dockManager);
    auto* sceneEditorDock = AddWindow<SceneEditorWindow>("Scene", m_dockManager);


	auto* rightArea0 = m_dockManager->addDockWidget(ads::RightDockWidgetArea, inspecterDock);
    auto* leftArea0 = m_dockManager->addDockWidget(ads::LeftDockWidgetArea, sceneEditorDock);

    auto* splitter = leftArea0->parentSplitter();

    int total = splitter->width();
    int left = total * 0.8;
    int right = total * 0.2;

    splitter->setSizes({ left, right });

    auto* bottomArea0 = m_dockManager->addDockWidget(ads::BottomDockWidgetArea, consoleDock, leftArea0);

	splitter = bottomArea0->parentSplitter();
	total = splitter->height();
    int top = total * 0.5;
    int bottom = total * 0.5;
	splitter->setSizes({ top, bottom });

    m_dockManager->addDockWidget(ads::RightDockWidgetArea, assetBrowserDock, bottomArea0);
}

LuxonEditor::GUI::QT::LuxonEditorWindow::~LuxonEditorWindow()
{}
