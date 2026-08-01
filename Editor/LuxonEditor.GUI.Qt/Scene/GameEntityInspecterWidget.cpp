#include "GameEntityInspecterWidget.h"
#include <QBoxLayout>
#include "../Core/TransformWidget.h"

namespace LuxonEditor::GUI::QT {
	GameEntityInspecterWidget::GameEntityInspecterWidget(QWidget* parent, ref<LuxonEngine::GameEntity>& entity)
		:m_entity(entity), QWidget(parent)
	{
		QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
		setLayout(layout);

		QWidget* transformPanel = new QWidget(this);
		QVBoxLayout* transformLayout = new QVBoxLayout();
		transformPanel->setLayout(transformLayout);
		layout->addWidget(transformPanel);
		transformPanel->setObjectName("transformPanel");
		transformPanel->setStyleSheet("#transformPanel { border: 1px solid #ffffff; border-radius: 5px; padding: 5px; }");

		QLabel* transformLabel = new QLabel("Transform", transformPanel);
		transformLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		transformLayout->addWidget(transformLabel);
		transformLayout->setAlignment(transformLabel, Qt::AlignTop);
		QFont font = transformLabel->font();
		font.setPointSize(14);
		transformLabel->setFont(font);
		transformLabel->setAlignment(Qt::AlignCenter);
		TransformWidget* transformWidget = new TransformWidget(this);
		transformWidget->SetTransform(m_entity->GetTransform());
		transformLayout->addWidget(transformWidget);
		transformLayout->setAlignment(transformWidget, Qt::AlignTop);








		layout->addStretch(1);
	}

	GameEntityInspecterWidget::~GameEntityInspecterWidget()
	{
	}
}
