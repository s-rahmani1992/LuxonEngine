#include "GameEntityInspecterWidget.h"
#include <QBoxLayout>
#include <qpushbutton.h>
#include "../Core/TransformWidget.h"
#include "../Renderer/MeshRendererWidget.h"
#include "../Renderer/RTComponentWidget.h"
#include "../Renderer/SplineRendererWidget.h"
#include "../Renderer/GBufferRendererWidget.h"
#include <qline.h>
#include <LuxonEditorAPI.h>
#include <QMenu>

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
		rendererSelectorLayout->setAlignment(Qt::AlignHCenter);
		rendererSelectorPanel->setLayout(rendererSelectorLayout);
		rendererLayout->addWidget(rendererSelectorPanel);
		rendererLayout->setAlignment(rendererSelectorPanel, Qt::AlignVCenter);

		m_addRendererButton = new QToolButton(this);
		m_addRendererButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		m_addRendererButton->setMinimumWidth(150);
		m_addRendererButton->setText("Add Renderer");
		m_addRendererButton->setPopupMode(QToolButton::MenuButtonPopup); // enables dropdown arrow

		m_addRendererButton->setStyleSheet(m_addRendererButton->styleSheet() + "QToolButton { background-color: #555555; color: white; border: 1px solid #666666; border-radius: 4px; padding: 4px 4px; }"
			"QToolButton:hover { background-color: #606060; border-color: #2a82da; }"
			"QToolButton:pressed { background-color: #2a82da; border-color: #2a82da; }"
			"QToolButton:disabled { background-color: #404040; color: #808080; border-color: #505050; }");

		QMenu* menu = new QMenu(m_addRendererButton);
		menu->setStyleSheet("QMenu { background-color: #2b2b2b; color: #ffffff; }"
			"QMenu::item:selected { background-color: #3c3c3c; }");

		menu->addAction("Mesh Renderer", [this]() {
			auto meshRenderer = std::make_shared<LuxonEngine::Rendering::MeshRenderer>(nullptr, nullptr);
			m_entity->SetRenderer(meshRenderer);
			GenerateRendererWidget(meshRenderer);
			});

		menu->addAction("Spline Renderer", [this]() {
			auto splineRenderer = std::make_shared<LuxonEngine::Rendering::SplineRenderer>(nullptr, std::vector<LuxonEngine::Vector3>{LuxonEngine::Vector3(0.0f), LuxonEngine::Vector3(0.0f), LuxonEngine::Vector3(0.0f)}, 1.0f, 10, 1.0f);
			m_entity->SetRenderer(splineRenderer);
			GenerateRendererWidget(splineRenderer);
			});

		menu->addAction("G Buffer Reflection Renderer", [this]() {
			auto gbufferRenderer = std::make_shared<LuxonEngine::Rendering::GBufferRTReflectionRenderer>(nullptr, nullptr);
			m_entity->SetRenderer(gbufferRenderer);
			GenerateRendererWidget(gbufferRenderer);
			});

		m_addRendererButton->setMenu(menu);

		rendererSelectorLayout->addWidget(m_addRendererButton);

		m_removeRendererButton = new QPushButton("Remove Renderer", rendererSelectorPanel);
		rendererSelectorLayout->addWidget(m_removeRendererButton);

		connect(m_removeRendererButton, &QPushButton::clicked, this, [this]() {
			m_entity->SetRenderer(nullptr);
			GenerateRendererWidget(nullptr);
			});

		QFrame* line = new QFrame(m_rendererPanel);
		line->setFrameShape(QFrame::HLine);
		line->setFrameShadow(QFrame::Sunken);
		rendererLayout->addWidget(line);

		GenerateRendererWidget(m_entity->GetRenderer());

		// Ray Tracing Component section
		m_rtPanel = new QWidget(this);
		m_rtPanel->setObjectName("rtPanel");
		m_rtPanel->setStyleSheet(m_rtPanel->styleSheet() + "#rtPanel { border: 1px solid #ffffff; border-radius: 5px; padding: 5px; }");
		QVBoxLayout* rtLayout = new QVBoxLayout();
		rtLayout->setContentsMargins(2, 2, 2, 2);
		rtLayout->setSpacing(2);
		m_rtPanel->setLayout(rtLayout);
		layout->addWidget(m_rtPanel);

		m_rtButton = new QPushButton(m_rtPanel);
		rtLayout->addWidget(m_rtButton);

		QFrame* rtLine = new QFrame(m_rtPanel);
		rtLine->setFrameShape(QFrame::HLine);
		rtLine->setFrameShadow(QFrame::Sunken);
		rtLayout->addWidget(rtLine);

		UpdateRTComponentSection();

		connect(m_rtButton, &QPushButton::clicked, this, [this]() {
			if (m_entity->GetRayTracingComponent() == nullptr) {
				auto rtComponent = std::make_shared<LuxonEngine::Rendering::RayTracingComponent>(nullptr, nullptr);
				m_entity->SetRayTracingComponent(rtComponent);
			}
			else {
				m_entity->SetRayTracingComponent(nullptr);
			}
			UpdateRTComponentSection();
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
			auto noRendererLabel = new QLabel("No Renderer is attached", m_rendererPanel);
			noRendererLabel->setContentsMargins(2, 2, 2, 2);
			noRendererLabel->setAlignment(Qt::AlignCenter);
			m_rendererWidget = noRendererLabel;
			m_rendererPanel->layout()->addWidget(m_rendererWidget);
			m_removeRendererButton->setVisible(false);
			m_addRendererButton->setVisible(true);
			return;
		}

		m_addRendererButton->setVisible(false);
		m_removeRendererButton->setVisible(true);
		auto meshRenderer = std::dynamic_pointer_cast<LuxonEngine::Rendering::MeshRenderer>(renderer);

		if(meshRenderer != nullptr) {
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

		auto splineRenderer = std::dynamic_pointer_cast<LuxonEngine::Rendering::SplineRenderer>(renderer);

		if(splineRenderer != nullptr) {
			auto splineRendererPanel = new QWidget(m_rendererPanel);
			auto splineRendererLayout = new QVBoxLayout();
			splineRendererPanel->setLayout(splineRendererLayout);
			splineRendererLayout->setContentsMargins(2, 2, 2, 2);
			auto splineRendererLabel = new QLabel("Spline Renderer", splineRendererPanel);
			splineRendererLabel->setAlignment(Qt::AlignCenter);
			QFont font = splineRendererLabel->font();
			font.setPointSize(14);
			splineRendererLabel->setFont(font);
			splineRendererPanel->layout()->addWidget(splineRendererLabel);
			splineRendererPanel->layout()->setAlignment(splineRendererLabel, Qt::AlignTop);
			auto* splineRendererWidget = new SplineRendererWidget(splineRendererPanel, splineRenderer);
			splineRendererPanel->layout()->addWidget(splineRendererWidget);
			splineRendererPanel->layout()->setAlignment(splineRendererWidget, Qt::AlignTop);
			m_rendererWidget = splineRendererPanel;
			m_rendererPanel->layout()->addWidget(m_rendererWidget);
			return;
		}

		auto gBufferRenderer = std::dynamic_pointer_cast<LuxonEngine::Rendering::GBufferRTReflectionRenderer>(renderer);

		if (gBufferRenderer != nullptr) {
			auto gBufferRendererPanel = new QWidget(m_rendererPanel);
			auto gBufferRendererLayout = new QVBoxLayout();
			gBufferRendererPanel->setLayout(gBufferRendererLayout);
			gBufferRendererLayout->setContentsMargins(2, 2, 2, 2);
			auto gBufferRendererLabel = new QLabel("G Buffer Renderer", gBufferRendererPanel);
			gBufferRendererLabel->setAlignment(Qt::AlignCenter);
			QFont font = gBufferRendererLabel->font();
			font.setPointSize(14);
			gBufferRendererLabel->setFont(font);
			gBufferRendererPanel->layout()->addWidget(gBufferRendererLabel);
			gBufferRendererPanel->layout()->setAlignment(gBufferRendererLabel, Qt::AlignTop);
			auto* gBufferRendererWidget = new GBufferRendererWidget(gBufferRendererPanel);
			gBufferRendererWidget->SetGBufferRenderer(gBufferRenderer);
			gBufferRendererPanel->layout()->addWidget(gBufferRendererWidget);
			gBufferRendererPanel->layout()->setAlignment(gBufferRendererWidget, Qt::AlignTop);
			m_rendererWidget = gBufferRendererPanel;
			m_rendererPanel->layout()->addWidget(m_rendererWidget);
			return;
		}
		m_rendererWidget = new QLabel("No GUI is available for this renderer", m_rendererPanel);
		m_rendererPanel->layout()->addWidget(m_rendererWidget);
	}

	void GameEntityInspecterWidget::UpdateRTComponentSection()
	{
		if (m_rtComponentWidget != nullptr) {
			m_rtComponentWidget->deleteLater();
			m_rtComponentWidget = nullptr;
		}

		auto rtComponent = m_entity->GetRayTracingComponent();

		if (rtComponent == nullptr) {
			m_rtButton->setText("Add Ray Tracing Component");
			auto noRTLabel = new QLabel("No Ray Tracing Component is attached", m_rtPanel);
			noRTLabel->setContentsMargins(2, 2, 2, 2);
			noRTLabel->setAlignment(Qt::AlignCenter);
			m_rtComponentWidget = noRTLabel;
		}
		else {
			m_rtButton->setText("Remove Ray Tracing Component");

			auto rtComponentLabel = new QLabel("Ray Tracing", m_rtPanel);
			rtComponentLabel->setAlignment(Qt::AlignCenter);
			QFont font = rtComponentLabel->font();
			font.setPointSize(14);
			rtComponentLabel->setFont(font);
			m_rtPanel->layout()->addWidget(rtComponentLabel);
			m_rtPanel->layout()->setAlignment(rtComponentLabel, Qt::AlignTop);

			auto rtWidget = new RTComponentWidget(m_rtPanel);
			rtWidget->SetRTComponent(rtComponent);
			m_rtComponentWidget = rtWidget;
		}

		m_rtPanel->layout()->addWidget(m_rtComponentWidget);
	}
}