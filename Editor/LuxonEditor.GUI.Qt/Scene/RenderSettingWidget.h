#pragma once
#include <EngineAPI.h>

#include <QWidget>

class RenderSettingWidget  : public QWidget
{
	Q_OBJECT

public:
	RenderSettingWidget(ref<LuxonEngine::Scene> scene, QWidget *parent);
	~RenderSettingWidget();

private:
	ref<LuxonEngine::Scene> m_scene;
};

