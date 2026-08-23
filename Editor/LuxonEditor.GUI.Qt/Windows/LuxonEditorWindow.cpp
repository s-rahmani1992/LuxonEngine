#include "LuxonEditorWindow.h"
#include <LuxonEditorAPI.h>
#include "Console/ConsoleLogWindow.h"
#include "AssetBrowser/AssetBrowserWindow.h"
#include "ShaderCreation/ShaderCreationWindow.h"
#include "Material/MaterialCreationWindow.h"
#include "Scene/CreateSceneWindow.h"
#include "InspecterWindow.h"
#include "SceneEditorWindow.h"
#include "SceneHierarchyWindow.h"
#include "gameWindow.h"

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
    connect(ui.saveMenuItem, &QAction::triggered, [this]() {
        EngineApplication::GetSceneManager()->SaveCurrentScene();
		});

    connect(ui.createSceneMenuItem, &QAction::triggered, [this]() {
        CreateSceneWindow createSceneWindow(this);
        createSceneWindow.exec();
        });

    connect(ui.runButton, &QPushButton::clicked, [this]() {
        GameWindow* gameWindow = new GameWindow(this, EngineApplication::GetSceneManager()->GetCurrentScene(), ui.renderModeBox->currentIndex());
		gameWindow->show();
		});

    EngineApplication::GetSceneManager()->RegisterSceneLoadedCallback([this](ref<LuxonEngine::Scene> scene) {
        UpdateRunPanel();
		});

    EngineApplication::GetSceneManager()->RegisterRenderSettingChangedCallback([this](ref<LuxonEngine::Scene> scene) {
        UpdateRunPanel();
        });

    UpdateRunPanel();

	static_cast<QHBoxLayout*>(ui.runPanel->layout())->insertStretch(0, 1);
	ui.runPanel->layout()->setAlignment(ui.runButton, Qt::AlignHCenter);
	ui.runPanel->layout()->setAlignment(ui.renderModeBox, Qt::AlignHCenter);
	static_cast<QHBoxLayout*>(ui.runPanel->layout())->addStretch(1);

    ads::CDockManager::setConfigFlag(ads::CDockManager::DefaultDockAreaButtons, false);
    m_dockManager = new ads::CDockManager(ui.windowDocker);
    ui.windowDocker->layout()->addWidget(m_dockManager);

    m_dockManager->setStyleSheet(m_dockManager->styleSheet() +
        "ads--CDockSplitter::handle:horizontal { width: 3px; background-color: #222222; image: none; }"
        "ads--CDockSplitter::handle:vertical   { height: 3px; background-color: #222222; image: none; }"
    );

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

	SetupMenuItems(m_consoleDock, ui.consoleWindowMenuItem);
	SetupMenuItems(m_inspecterDock, ui.inspecterMenuItem);
	SetupMenuItems(m_assetBrowserDock, ui.assetBrowserMenuItem);
	SetupMenuItems(m_sceneEditorDock, ui.sceneViewMenuItem);
	SetupMenuItems(m_sceneHierarchyDock, ui.sceneHierarchyMenuItem);
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

void LuxonEditor::GUI::QT::LuxonEditorWindow::OnMenuItemTriggered(ads::CDockWidget* dockWidget, QAction* menuItem)
{
    if (!dockWidget->isClosed())
        return;

    dockWidget->toggleView(true);
	UpdateDockWidgetMenuItemState(dockWidget, menuItem);
}

void LuxonEditor::GUI::QT::LuxonEditorWindow::UpdateDockWidgetMenuItemState(ads::CDockWidget* dockWidget, QAction* menuItem)
{
    menuItem->setChecked(!dockWidget->isClosed());
}

void LuxonEditor::GUI::QT::LuxonEditorWindow::UpdateRunPanel()
{
	auto scene = EngineApplication::GetSceneManager()->GetCurrentScene();

	ui.renderModeBox->clear();

    if (scene->canSupportHybridRendering) {
		ui.renderModeBox->addItem("Hybrid", QVariant::fromValue(1));
    }

    if (scene->canSupportRayTracing) {
        ui.renderModeBox->addItem("Ray Tracing", QVariant::fromValue(2));
    }

	ui.renderModeBox->setCurrentIndex(ui.renderModeBox->count() > 0 ? 0 : -1);
	ui.runButton->setEnabled(ui.renderModeBox->count() > 0);
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

	UpdateDockWidgetMenuItemState(m_consoleDock, ui.consoleWindowMenuItem);
	UpdateDockWidgetMenuItemState(m_inspecterDock, ui.inspecterMenuItem);
	UpdateDockWidgetMenuItemState(m_assetBrowserDock, ui.assetBrowserMenuItem);
	UpdateDockWidgetMenuItemState(m_sceneEditorDock, ui.sceneViewMenuItem);
	UpdateDockWidgetMenuItemState(m_sceneHierarchyDock, ui.sceneHierarchyMenuItem);
}

void LuxonEditor::GUI::QT::LuxonEditorWindow::SetupMenuItems(ads::CDockWidget* dockWidget, QAction* menuItem)
{
    connect(menuItem, &QAction::triggered, [this, dockWidget, menuItem]() {
        OnMenuItemTriggered(dockWidget, menuItem);
        });

    connect(dockWidget, &ads::CDockWidget::closed, [this, dockWidget, menuItem]() {
        UpdateDockWidgetMenuItemState(dockWidget, menuItem);
        });
    
	UpdateDockWidgetMenuItemState(dockWidget, menuItem);
}
