#include "GameEntityInspecterWidget.h"
#include <QBoxLayout>
#include <qpushbutton.h>
#include "../Core/TransformWidget.h"
#include "../Renderer/MeshRendererWidget.h"
#include <qline.h>
#include <LuxonEditorAPI.h>

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

		connect(transformWidget, &TransformWidget::ValueChanged, this, [this]() {
			EngineApplication::GetSceneManager()->RequestRender();
			});

		m_rendererPanel = new QWidget(this);
		m_rendererPanel->setObjectName("rendererPanel");
		m_rendererPanel->setStyleSheet(m_rendererPanel->styleSheet() + "#rendererPanel {border: 1px solid #ffffff; border-radius: 5px; padding: 5px; }");	
		QVBoxLayout* rendererLayout = new QVBoxLayout();
		rendererLayout->setContentsMargins(2, 2, 2, 2);
		rendererLayout->setSpacing(2);
		m_rendererPanel->setLayout(rendererLayout);
		layout->addWidget(m_rendererPanel);

		QWidget* rendererSelectorPanel = new QWidget(m_rendererPanel);
		QHBoxLayout* rendererSelectorLayout = new QHBoxLayout();
		rendererSelectorLayout->setContentsMargins(4, 4, 4, 4);
		rendererSelectorPanel->setLayout(rendererSelectorLayout);
		rendererLayout->addWidget(rendererSelectorPanel);
		rendererLayout->setAlignment(rendererSelectorPanel, Qt::AlignVCenter);
		m_rendererTypeComboBox = new QComboBox(rendererSelectorPanel);
		rendererSelectorLayout->addWidget(m_rendererTypeComboBox);
		m_rendererTypeComboBox->setPlaceholderText("Select Renderer Type");
		m_rendererTypeComboBox->addItem("MeshRenderer");
		rendererSelectorLayout->setAlignment(m_rendererTypeComboBox, Qt::AlignLeft | Qt::AlignVCenter);
		QPushButton* addRendererButton = new QPushButton("Add", rendererSelectorPanel);
		rendererSelectorLayout->addWidget(addRendererButton);
		rendererSelectorLayout->setAlignment(addRendererButton, Qt::AlignLeft | Qt::AlignVCenter);

		QFrame* line = new QFrame(m_rendererPanel);
		line->setFrameShape(QFrame::HLine);
		line->setFrameShadow(QFrame::Sunken);
		rendererLayout->addWidget(line);

		GenerateRendererWidget(m_entity->GetRenderer());
		addRendererButton->setEnabled(m_rendererTypeComboBox->currentIndex() != m_rendererTypeIndex);
		
		connect(addRendererButton, &QPushButton::clicked, this, [this]() {
			if (m_rendererTypeComboBox->currentIndex() == 0) {
				auto meshRenderer = std::make_shared<LuxonEngine::Rendering::MeshRenderer>(nullptr, nullptr);
				m_entity->SetRenderer(meshRenderer);
				GenerateRendererWidget(meshRenderer);
			}
			});

		connect(m_rendererTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, addRendererButton](int index) {
			addRendererButton->setEnabled(index != m_rendererTypeIndex);
			});

		
		
		layout->addStretch(1);
	}

	GameEntityInspecterWidget::~GameEntityInspecterWidget()
	{
	}

	void GameEntityInspecterWidget::GenerateRendererWidget(ref<LuxonEngine::Rendering::Renderer> renderer)
	{
		if(m_rendererWidget != nullptr) {
			m_rendererWidget->deleteLater();
			m_rendererWidget = nullptr;
		}

		if (renderer == nullptr) {
			m_rendererTypeIndex = -1;
			m_rendererTypeComboBox->setCurrentIndex(-1);
			auto noRendererLabel = new QLabel("No Renderer is attached", m_rendererPanel);
			noRendererLabel->setContentsMargins(2, 2, 2, 2);
			noRendererLabel->setAlignment(Qt::AlignCenter);
			m_rendererWidget = noRendererLabel;
			m_rendererPanel->layout()->addWidget(m_rendererWidget);
			return;
		}

		auto meshRenderer = std::dynamic_pointer_cast<LuxonEngine::Rendering::MeshRenderer>(renderer);

		if(meshRenderer != nullptr) {
			m_rendererTypeIndex = 0;
			m_rendererTypeComboBox->setCurrentIndex(0);

			auto meshRendererPanel = new QWidget(m_rendererPanel);
			auto meshRendererLayout = new QVBoxLayout();
			meshRendererPanel->setLayout(meshRendererLayout);
			meshRendererLayout->setContentsMargins(2, 2, 2, 2);
			auto meshRendererLabel = new QLabel("Mesh Renderer", meshRendererPanel);
			meshRendererLabel->setAlignment(Qt::AlignCenter);
			QFont font = meshRendererLabel->font();
			font.setPointSize(14);
			meshRendererLabel->setFont(font);
			meshRendererPanel->layout()->addWidget(meshRendererLabel);
			meshRendererPanel->layout()->setAlignment(meshRendererLabel, Qt::AlignTop);

			auto* meshRendererWidget = new MeshRendererWidget(meshRendererPanel);
			meshRendererWidget->SetMeshRenderer(meshRenderer);
			meshRendererPanel->layout()->addWidget(meshRendererWidget);
			meshRendererPanel->layout()->setAlignment(meshRendererWidget, Qt::AlignTop);
			m_rendererWidget = meshRendererPanel;
			m_rendererPanel->layout()->addWidget(m_rendererWidget);
			return;
		}

		m_rendererWidget = new QLabel("No GUI is available for this renderer", m_rendererPanel);
		m_rendererPanel->layout()->addWidget(m_rendererWidget);
	}
}
