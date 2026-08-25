#pragma once

#include <QWidget>
#include <LuxonEditorAPI.h>

class EntityRotatorWidget  : public QWidget
{
	Q_OBJECT

public:
	EntityRotatorWidget(QWidget *parent, ref<EntityRotator> rotator);
	~EntityRotatorWidget();

private:
	ref<EntityRotator> m_rotator;
};

