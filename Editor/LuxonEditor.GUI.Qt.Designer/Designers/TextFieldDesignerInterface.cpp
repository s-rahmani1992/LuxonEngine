#include "TextFieldDesignerInterface.h"
#include <QtCore/QtPlugin>
#include <Widgets/QTextField.h>

TextFieldDesignerInterface::TextFieldDesignerInterface(QObject *parent)
	: QObject(parent)
{
	initialized = false;
}

TextFieldDesignerInterface::~TextFieldDesignerInterface()
{}

bool TextFieldDesignerInterface::isContainer() const
{
	return false;
}

bool TextFieldDesignerInterface::isInitialized() const
{
	return initialized;
}

QString TextFieldDesignerInterface::group() const
{
	return "Data Field Widgets";
}

QString TextFieldDesignerInterface::includeFile() const
{
	return "QTextField.h";
}

QIcon TextFieldDesignerInterface::icon() const
{
	return QIcon();
}

QString TextFieldDesignerInterface::domXml() const
{
	return R"(
        <ui language="c++">
            <widget class="QTextField" name="textField">
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

QString TextFieldDesignerInterface::name() const
{
	return "QTextField";
}

QString TextFieldDesignerInterface::toolTip() const
{
	return "Field for holding string value";
}

QString TextFieldDesignerInterface::whatsThis() const
{
	return QString();
}

QWidget* TextFieldDesignerInterface::createWidget(QWidget* parent)
{
	return new QTextField(parent);
}

void TextFieldDesignerInterface::initialize(QDesignerFormEditorInterface* core)
{
	if (initialized)
		return;

	initialized = true;
}

