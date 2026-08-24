#pragma once

#include <QWidget>
#include <LuxonEditorAPI.h>

class MaterialValueModifierWidget  : public QWidget
{
	Q_OBJECT

public:
	MaterialValueModifierWidget(QWidget *parent, ref<MaterialValueModifier> materialValueModifier);
	~MaterialValueModifierWidget();

private:
	ref<MaterialValueModifier> m_materialValueModifier;
};

