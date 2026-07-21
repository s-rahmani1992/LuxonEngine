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

private:
	Ui::MaterialInspecterWidgetClass ui;
};

