#pragma once

#include <QWidget>
#include <QComboBox>
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

		ref<LuxonEngine::GameEntity> m_entity;
		QWidget* m_rendererWidget = nullptr;

		QWidget* m_rendererPanel = nullptr;
		QComboBox* m_rendererTypeComboBox = nullptr;

		int m_rendererTypeIndex = -1;
	};
}
