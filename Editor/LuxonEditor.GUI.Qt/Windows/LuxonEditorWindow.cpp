#include "LuxonEditorWindow.h"
#include <LuxonEditorAPI.h>
#include "Console/ConsoleLogWindow.h"
#include "AssetBrowser/AssetBrowserWindow.h"
#include "ShaderCreation/ShaderCreationWindow.h"
#include "Material/MaterialCreationWindow.h"
#include "InspecterWindow.h"
#include "SceneEditorWindow.h"
#include "SceneHierarchyWindow.h"

//TODO Right now, The ADS docking is copied into this project. use the git submodule in the future
#include "../ADS/src/DockSplitter.h"

#include <QSettings>
#include <QCloseEvent>
#include <QTimer>

static const QString k_dockStateKey = "LuxonEditor/DockManagerState";

LuxonEditor::GUI::QT::LuxonEditorWindow::LuxonEditorWindow(QWidget* parent)
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

    m_consoleDock = AddWindow<ConsoleLogWindow>("Console", m_dockManager);
    m_assetBrowserDock = AddWindow<AssetBrowserWindow>("Browser", QString::fromStdString(GetProjectPath() + "/Assets"), QString::fromStdString(GetProjectPath() + "/Assets"));
    m_inspecterDock = AddWindow<InspecterWindow>("Inspecter", m_dockManager);
    m_sceneEditorDock = AddWindow<SceneEditorWindow>("Scene", m_dockManager);
    m_sceneHierarchyDock = AddWindow<SceneHierarchyWindow>("Hierarchy", m_dockManager);

    // Always build the default layout first so every CDockWidget is
    // registered in DockWidgetsMap before any restore attempt.
    setupDefaultDockLayout();

    // Defer restore until after show() so the dock manager has real dimensions.
    QTimer::singleShot(0, this, &LuxonEditorWindow::restoreDockState);
}

LuxonEditor::GUI::QT::LuxonEditorWindow::~LuxonEditorWindow()
{
}

void LuxonEditor::GUI::QT::LuxonEditorWindow::closeEvent(QCloseEvent* event)
{
    QSettings settings;
    settings.setValue(k_dockStateKey, m_dockManager->saveState());
    QMainWindow::closeEvent(event);
}

void LuxonEditor::GUI::QT::LuxonEditorWindow::setupDefaultDockLayout()
{
    auto* rightArea0 = m_dockManager->addDockWidget(ads::RightDockWidgetArea, m_inspecterDock);
    auto* leftArea0 = m_dockManager->addDockWidget(ads::LeftDockWidgetArea, m_sceneEditorDock);

    auto* splitter = leftArea0->parentSplitter();
    int total = splitter->width();
    splitter->setSizes({ (int)(total * 0.7), (int)(total * 0.3) });

    auto* bottomArea0 = m_dockManager->addDockWidget(ads::BottomDockWidgetArea, m_consoleDock, leftArea0);
    splitter = bottomArea0->parentSplitter();
    total = splitter->height();
    splitter->setSizes({ (int)(total * 0.5), (int)(total * 0.5) });

    auto* bottomRightArea = m_dockManager->addDockWidget(ads::RightDockWidgetArea, m_assetBrowserDock, bottomArea0);
    splitter = bottomRightArea->parentSplitter();
    total = splitter->width();
    splitter->setSizes({ (int)(total * 0.5), (int)(total * 0.5) });

    m_dockManager->addDockWidget(ads::LeftDockWidgetArea, m_sceneHierarchyDock, leftArea0);
}

void LuxonEditor::GUI::QT::LuxonEditorWindow::restoreDockState()
{
    QSettings settings;
    if (settings.contains(k_dockStateKey))
    {
        QByteArray savedState = settings.value(k_dockStateKey).toByteArray();
        if (!m_dockManager->restoreState(savedState))
        {
            // Saved state was invalid or from an older layout — discard it.
            settings.remove(k_dockStateKey);
        }
    }
}