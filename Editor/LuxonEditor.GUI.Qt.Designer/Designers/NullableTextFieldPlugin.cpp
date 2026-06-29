
#include "NullableTextFieldPlugin.h"

#include <QtCore/QtPlugin>
#include <Widgets/NullableTextField.h>

QNullableTextFieldPlugin::QNullableTextFieldPlugin(QObject *parent)
    : QObject(parent)
{
    initialized = false;
}

void QNullableTextFieldPlugin::initialize(QDesignerFormEditorInterface * /*core*/)
{
    if (initialized)
        return;

    initialized = true;
}

bool QNullableTextFieldPlugin::isInitialized() const
{
    return initialized;
}

QWidget *QNullableTextFieldPlugin::createWidget(QWidget *parent)
{
    return new QNullableTextField(parent, true);
}

QString QNullableTextFieldPlugin::name() const
{
    return "QNullableTextField";
}

QString QNullableTextFieldPlugin::group() const
{
    return "Data Field Widgets";
}

QIcon QNullableTextFieldPlugin::icon() const
{
    return QIcon();
}

QString QNullableTextFieldPlugin::toolTip() const
{
    return "Field for holding string value which can be toggled to be null";
}

QString QNullableTextFieldPlugin::whatsThis() const
{
    return QString();
}

bool QNullableTextFieldPlugin::isContainer() const
{
    return false;
}

QString QNullableTextFieldPlugin::domXml() const
{
    return R"(
        <ui language="c++">
            <widget class="QNullableTextField" name="nullableTextField">
                <property name="labelText">
                    <string>LabelName</string>
                </property>
                <property name="placeHolder">
                    <string>Your Text</string>
                </property>
            </widget>
        </ui>
    )";
}

QString QNullableTextFieldPlugin::includeFile() const
{
    return "NullableTextField.h";
}
