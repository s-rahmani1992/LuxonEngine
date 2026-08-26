#include "BehaviourListWidget.h"
#include "Behaviours/BehaviourItemWidget.h"
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qmenu.h>
#include <LuxonEditorAPI.h>

BehaviourListWidget::BehaviourListWidget(QWidget *parent, ref<LuxonEngine::Scene> scene)
	: QWidget(parent)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(2, 2, 2, 2);
	layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	setLayout(layout);

	setStyleSheet(styleSheet() + R"(
		QToolButton {
            background-color: #555555;
            color: white;
            border: 1px solid #666666;
            border-radius: 4px;
            padding: 4px 4px;
        }

        ToolButton:hover {
            background-color: #606060;
            border-color: #2a82da;
        }

        ToolButton:pressed {
            background-color: #2a82da;
            border-color: #2a82da;
        }

        ToolButton:disabled {
            background-color: #404040;
            color: #808080;
            border-color: #505050;
        }
	)");

	m_behaviourContainer = new QWidget(this);
	m_behaviourContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_containerLayout = new QVBoxLayout(m_behaviourContainer);
	m_containerLayout->setContentsMargins(0, 0, 0, 0);
	m_containerLayout->setAlignment(Qt::AlignTop);
	m_behaviourContainer->setLayout(m_containerLayout);
	layout->addWidget(m_behaviourContainer);

	for(auto& behaviour : scene->behaviours) {
		AddWidgetForBehaviour(behaviour);
	}

	QToolButton* addBehaviourButton = new QToolButton(this);
	addBehaviourButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	addBehaviourButton->setText("Add Custom Behaviour");
	addBehaviourButton->setPopupMode(QToolButton::MenuButtonPopup); // enables dropdown arrow

	QMenu* menu = new QMenu(addBehaviourButton);
	menu->setStyleSheet("QMenu { background-color: #2b2b2b; color: #ffffff; }"
		"QMenu::item:selected { background-color: #3c3c3c; }");

	menu->addAction("Basic Camera Navigator", [this]() {
		auto newBehaviour = LuxonEditor::EngineApplication::GetSceneManager()->AddBehaviour<BasicCameraNavigator>();
		AddWidgetForBehaviour(newBehaviour);
		});

	menu->addAction("Material Value Modifier", [this]() {
		auto newBehaviour = LuxonEditor::EngineApplication::GetSceneManager()->AddBehaviour<MaterialValueModifier>();
		AddWidgetForBehaviour(newBehaviour);		
		});

	menu->addAction("Entity Rotator", [this]() {
		auto newBehaviour = LuxonEditor::EngineApplication::GetSceneManager()->AddBehaviour<EntityRotator>();
		AddWidgetForBehaviour(newBehaviour);
		});

	menu->addAction("Entity Position Controller", [this]() {
		auto newBehaviour = LuxonEditor::EngineApplication::GetSceneManager()->AddBehaviour<EntityPositionController>();
		AddWidgetForBehaviour(newBehaviour);
		});

	menu->addAction("Entity Mover", [this]() {
		auto newBehaviour = LuxonEditor::EngineApplication::GetSceneManager()->AddBehaviour<EntityMover>();
		AddWidgetForBehaviour(newBehaviour);
		});

	addBehaviourButton->setMenu(menu);
	layout->addWidget(addBehaviourButton);
}

BehaviourListWidget::~BehaviourListWidget()
{}

void BehaviourListWidget::AddWidgetForBehaviour(ref<LuxonEngine::Behaviour> behaviour)
{
	BehaviourItemWidget* behaviourItemWidget = new BehaviourItemWidget(m_behaviourContainer, behaviour);
	behaviourItemWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_containerLayout->addWidget(behaviourItemWidget);
}

