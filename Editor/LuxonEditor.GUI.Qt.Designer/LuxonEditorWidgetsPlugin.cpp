#include "LuxonEditorWidgetsPlugin.h"
#include "Designers/TextFieldDesignerInterface.h"
#include "Designers/NullableTextFieldPlugin.h"
#include "Designers/FloatFieldDesignInterface.h"

LuxonEditorWidgetsPlugin::LuxonEditorWidgetsPlugin(QObject* parent)
    : QObject(parent)
{
    m_widgets.append(new TextFieldDesignerInterface(this));
    m_widgets.append(new QNullableTextFieldPlugin(this));
	m_widgets.append(new FloatFieldDesignInterface(this));
}

QList<QDesignerCustomWidgetInterface*> LuxonEditorWidgetsPlugin::customWidgets() const
{
    return m_widgets;
}