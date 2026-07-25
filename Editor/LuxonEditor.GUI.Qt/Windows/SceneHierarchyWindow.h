#pragma once

#include <QWidget>
#include "ui_SceneHierarchyWindow.h"

class SceneHierarchyWindow : public QWidget
{
	Q_OBJECT

public:
	SceneHierarchyWindow(QWidget *parent = nullptr);
	~SceneHierarchyWindow();

private:
	Ui::SceneHierarchyWindowClass ui;
};

