#pragma once

#include <QWidget>
#include <LuxonEditorAPI.h>

class EntityPositionControllerWidget  : public QWidget
{
	Q_OBJECT

public:
	EntityPositionControllerWidget(QWidget *parent, ref<EntityPositionController> entityPositionController);
	~EntityPositionControllerWidget();

private:
	ref<EntityPositionController> m_entityPositionController;
};

