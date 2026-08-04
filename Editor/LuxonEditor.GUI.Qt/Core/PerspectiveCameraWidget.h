#pragma once

#include <QWidget>
#include <EngineAPI.h>
#include <Core/Camera/PerspectiveCamera.h>
#include "TransformWidget.h"
#include <Widgets/QFloatField.h>

class PerspectiveCameraWidget : public QWidget
{
	Q_OBJECT

public:
	PerspectiveCameraWidget(QWidget* parent = nullptr);
	~PerspectiveCameraWidget();

	void SetCamera(const ref<LuxonEngine::PerspectiveCamera>& camera);

Q_SIGNALS:
	void ValueChanged();

private:
	TransformWidget* m_transformWidget;
	QFloatField* m_nearZField;
	QFloatField* m_farZField;
	QFloatField* m_fovAngleField;

	ref<LuxonEngine::PerspectiveCamera> m_camera;
};