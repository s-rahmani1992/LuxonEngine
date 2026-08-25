#pragma once

#include <QWidget>
#include <LuxonEditorAPI.h>

class EntityMoverWidget  : public QWidget
{
	Q_OBJECT

public:
	EntityMoverWidget(QWidget* parent, ref<EntityMover> entityMover);
	~EntityMoverWidget();
private:
	ref<EntityMover> m_entityMover;
};

