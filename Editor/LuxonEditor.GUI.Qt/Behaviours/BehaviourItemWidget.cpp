#include "BehaviourItemWidget.h"
#include <LuxonEditorAPI.h>
#include "MaterialValueModifierWidget.h"
#include "BasicCameraNavigatorWidget.h"
#include "EntityRotatorWidget.h"
#include "EntityPositionControllerWidget.h"
#include "EntityMoverWidget.h"
#include <QLabel>
#include <QBoxLayout>
#include <QFont>
#include <QPushButton>

BehaviourItemWidget::BehaviourItemWidget(QWidget* parent, ref<LuxonEngine::Behaviour> behaviour)
	: QWidget(parent), m_behaviour(behaviour)
{
	QBoxLayout* mainLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	mainLayout->setContentsMargins(2, 2, 2, 2);
	mainLayout->setAlignment(Qt::AlignTop);
	setLayout(mainLayout);
	setObjectName("behaviourItemWidget");
	setAttribute(Qt::WA_StyledBackground, true);

	this->setStyleSheet(styleSheet() + "#behaviourItemWidget { border: 1px solid #ffffff; border-radius: 5px; padding: 2px; }");

	QWidget* titlePanel = new QWidget(this);
	QHBoxLayout* titleLayout = new QHBoxLayout();
	titleLayout->setAlignment(Qt::AlignLeft);
	titleLayout->setContentsMargins(2, 2, 2, 2);
	titlePanel->setLayout(titleLayout);

	QLabel* titleLabel = new QLabel("Behaviour", titlePanel);
	titleLayout->addWidget(titleLabel);

	QPushButton* removeButton = new QPushButton("Remove", titlePanel);
	titleLayout->addWidget(removeButton);
	titleLayout->setAlignment(removeButton, Qt::AlignRight);

	connect(removeButton, &QPushButton::clicked, this, [this]() {
		LuxonEditor::EngineApplication::GetSceneManager()->RemoveBehaviour(m_behaviour);
		this->deleteLater();
		});

	mainLayout->addWidget(titlePanel);

	QFrame* rtLine = new QFrame(this);
	rtLine->setFrameShape(QFrame::HLine);
	rtLine->setFrameShadow(QFrame::Sunken);
	mainLayout->addWidget(rtLine);

	QWidget* behaviourContainer = new QWidget(this);
	behaviourContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	QBoxLayout* behaviourLayout = new QBoxLayout(QBoxLayout::TopToBottom, behaviourContainer);
	behaviourLayout->setContentsMargins(2, 2, 2, 2);
	behaviourContainer->setLayout(behaviourLayout);
	mainLayout->addWidget(behaviourContainer);

	auto addBehaviourWidget = [behaviourLayout](QWidget* widget) {
		widget->setSizePolicy(QSizePolicy::Expanding, widget->sizePolicy().verticalPolicy());
		behaviourLayout->addWidget(widget);
		};

	auto materialValueModifier = std::dynamic_pointer_cast<MaterialValueModifier>(behaviour);

	if (materialValueModifier != nullptr) {
		MaterialValueModifierWidget* materialValueWidget = new MaterialValueModifierWidget(behaviourContainer, materialValueModifier);
		titleLabel->setText("Material Value Modifier");
		addBehaviourWidget(materialValueWidget);
		return;
	}

	if (auto cameraNavigator = std::dynamic_pointer_cast<BasicCameraNavigator>(behaviour)) {
		titleLabel->setText("Basic Camera Navigator");
		BasicCameraNavigatorWidget* cameraNavigatorWidget = new BasicCameraNavigatorWidget(behaviourContainer, cameraNavigator);
		addBehaviourWidget(cameraNavigatorWidget);
		return;
	}

	if (auto entityRotator = std::dynamic_pointer_cast<EntityRotator>(behaviour)) {
		titleLabel->setText("Entity Rotator");
		EntityRotatorWidget* entityRotatorWidget = new EntityRotatorWidget(behaviourContainer, entityRotator);
		addBehaviourWidget(entityRotatorWidget);
		return;
	}

	if (auto entityPositionController = std::dynamic_pointer_cast<EntityPositionController>(behaviour)) {
		titleLabel->setText("Entity Position Controller");
		EntityPositionControllerWidget* entityPositionControllerWidget = new EntityPositionControllerWidget(behaviourContainer, entityPositionController);
		addBehaviourWidget(entityPositionControllerWidget);
		return;
	}

	if (auto entityMover = std::dynamic_pointer_cast<EntityMover>(behaviour)) {
		titleLabel->setText("Entity Mover");
		EntityMoverWidget* entityMoverWidget = new EntityMoverWidget(behaviourContainer, entityMover);
		addBehaviourWidget(entityMoverWidget);
		return;
	}
}
BehaviourItemWidget::~BehaviourItemWidget()
{}

