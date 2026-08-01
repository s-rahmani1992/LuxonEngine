#pragma once

#include <QWidget>
#include <EngineAPI.h>

namespace LuxonEditor::GUI::QT {
	class GameEntityInspecterWidget : public QWidget
	{
		Q_OBJECT

	public:
		GameEntityInspecterWidget(QWidget* parent, ref<LuxonEngine::GameEntity>& entity);
		~GameEntityInspecterWidget();

	private:
		ref<LuxonEngine::GameEntity> m_entity;
	};
}
