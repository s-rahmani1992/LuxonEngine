#include "FloatFieldDesignInterface.h"
#include "FloatFieldDesignInterface.h"
#include <QtCore/QtPlugin>
#include <Widgets/QFloatField.h>

FloatFieldDesignInterface::FloatFieldDesignInterface(QObject* parent)
	: QObject(parent)
{
	initialized = false;
}

FloatFieldDesignInterface::~FloatFieldDesignInterface()
{}

bool FloatFieldDesignInterface::isContainer() const
{
	return false;
}

bool FloatFieldDesignInterface::isInitialized() const
{
	return initialized;
}

QString FloatFieldDesignInterface::group() const
{
	return "Data Field Widgets";
}

QString FloatFieldDesignInterface::includeFile() const
{
	return "QFloatField.h";
}

QIcon FloatFieldDesignInterface::icon() const
{
	return QIcon();
}

QString FloatFieldDesignInterface::domXml() const
{
	return R"(
		<ui language="c++">
			<widget class="QFloatField" name="floatField">
				<property name="labelText">
					<string>LabelName</string>
				</property>
				<property name="value">
					<double>0.0</double>
				</property>
			</widget>
		</ui>
	)";
}

QString FloatFieldDesignInterface::name() const
{
	return "QFloatField";
}

QString FloatFieldDesignInterface::toolTip() const
{
	return "Field for holding float value";
}

QString FloatFieldDesignInterface::whatsThis() const
{
	return QString();
}

QWidget* FloatFieldDesignInterface::createWidget(QWidget* parent)
{
	return new QFloatField(parent);
}

void FloatFieldDesignInterface::initialize(QDesignerFormEditorInterface* core)
{
	if (initialized)
		return;

	initialized = true;
}

