#pragma once

#include <QWidget>
#include "ui_MaterialInspecterWidget.h"
#include <LuxonEditorAPI.h>

class MaterialInspecterWidget : public QWidget
{
	Q_OBJECT

public:
	MaterialInspecterWidget(QWidget *parent = nullptr, LuxonEngine::SerializationStream * stream = nullptr, std::string path = "");
	~MaterialInspecterWidget();

protected:
	virtual bool eventFilter(QObject* obj, QEvent* event) override;
private:
	Ui::MaterialInspecterWidgetClass ui;

	ref<Scene> m_scene;

	ref<LuxonEngine::Rendering::Material> m_material;
	ref<LuxonEngine::Rendering::GraphicContext> m_context;
	ref<LuxonEngine::Platform::GraphicWindow> m_window;
};

