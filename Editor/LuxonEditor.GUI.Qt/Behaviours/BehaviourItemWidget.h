#pragma once

#include <QWidget>
#include <EngineAPI.h>

class BehaviourItemWidget  : public QWidget
{
	Q_OBJECT

public:
	BehaviourItemWidget(QWidget *parent, ref<LuxonEngine::Behaviour> behaviour);
	~BehaviourItemWidget();

private:
	ref<LuxonEngine::Behaviour> m_behaviour;
};

