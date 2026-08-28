#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <qtoolbutton.h>
#include <EngineAPI.h>

namespace LuxonEditor::GUI::QT {
	class GameEntityInspecterWidget : public QWidget
	{
		Q_OBJECT

	public:
		GameEntityInspecterWidget(QWidget* parent, ref<LuxonEngine::GameEntity>& entity);
		~GameEntityInspecterWidget();

	private:
		void GenerateRendererWidget(ref<LuxonEngine::Rendering::Renderer> renderer);
		void UpdateRTComponentSection();

		ref<LuxonEngine::GameEntity> m_entity;

		QPushButton* m_removeRendererButton;
		QToolButton* m_addRendererButton;
		QWidget* m_rendererWidget = nullptr;
		QWidget* m_rendererPanel = nullptr;

		QWidget* m_rtPanel = nullptr;
		QPushButton* m_rtButton = nullptr;
		QWidget* m_rtComponentWidget = nullptr;
	};
}