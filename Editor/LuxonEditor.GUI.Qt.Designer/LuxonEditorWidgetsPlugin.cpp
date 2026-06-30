#include "LuxonEditorWidgetsPlugin.h"
#include "Designers/TextFieldDesignerInterface.h"
#include "Designers/NullableTextFieldPlugin.h"  // or whatever your other widget is

LuxonEditorWidgetsPlugin::LuxonEditorWidgetsPlugin(QObject* parent)
    : QObject(parent)
{
    m_widgets.append(new TextFieldDesignerInterface(this));
    m_widgets.append(new QNullableTextFieldPlugin(this));
}

QList<QDesignerCustomWidgetInterface*> LuxonEditorWidgetsPlugin::customWidgets() const
{
    return m_widgets;
}