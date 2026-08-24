#pragma once

#include <QWidget>
#include <LuxonEditorAPI.h>

class BasicCameraNavigatorWidget  : public QWidget
{
	Q_OBJECT

public:
	BasicCameraNavigatorWidget(QWidget *parent, ref<BasicCameraNavigator> cameraNavigator);
	~BasicCameraNavigatorWidget();

private:
	ref<BasicCameraNavigator> m_cameraNavigator;
};

