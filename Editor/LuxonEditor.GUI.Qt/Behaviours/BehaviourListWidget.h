#pragma once

#include <QWidget>
#include <qboxlayout.h>
#include <EngineAPI.h>

class BehaviourListWidget  : public QWidget
{
	Q_OBJECT

public:
	BehaviourListWidget(QWidget *parent, ref<LuxonEngine::Scene> scene);
	~BehaviourListWidget();

private:
	void AddWidgetForBehaviour(ref<LuxonEngine::Behaviour> behaviour);

	QWidget* m_behaviourContainer;
	QVBoxLayout* m_containerLayout;
};

