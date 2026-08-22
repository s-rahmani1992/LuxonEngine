#include "QVector3Field.h"

QVector3Field::QVector3Field(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.fields->layout()->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	//ui.fields->layout()->setAlignment(ui.yField, Qt::AlignTop);
	//ui.fields->layout()->setAlignment(ui.zField, Qt::AlignTop);

	ui.label->setStyleSheet(ui.label->styleSheet() + "#label { padding-top: 5px; }");

	ui.horizontalLayout->setAlignment(ui.label, Qt::AlignTop | Qt::AlignVCenter);
	//dd
	connect(ui.xField, &QFloatField::ValueChanged, this, &QVector3Field::OnXChanged);
	connect(ui.yField, &QFloatField::ValueChanged, this, &QVector3Field::OnYChanged);
	connect(ui.zField, &QFloatField::ValueChanged, this, &QVector3Field::OnZChanged);
}

QVector3Field::~QVector3Field()
{}

QString QVector3Field::labelText() const
{
	return ui.label->text();
}

void QVector3Field::setLabelText(const QString& t)
{
	ui.label->setText(t);
}

double QVector3Field::valueX() const
{
	return ui.xField->value();
}

void QVector3Field::setValueX(double v)
{
	ui.xField->setValue(v);
}

double QVector3Field::valueY() const
{
	return ui.yField->value();
}

void QVector3Field::setValueY(double v)
{
	ui.yField->setValue(v);
}

double QVector3Field::valueZ() const
{
	return ui.zField->value();
}

void QVector3Field::setValueZ(double v)
{
	ui.zField->setValue(v);
}

LuxonEngine::Vector3 QVector3Field::value() const
{
	return LuxonEngine::Vector3(
		(float)ui.xField->value(),
		(float)ui.yField->value(),
		(float)ui.zField->value()
	);
}

void QVector3Field::setValue(const LuxonEngine::Vector3& v)
{
	ui.xField->setValue(v.x);
	ui.yField->setValue(v.y);
	ui.zField->setValue(v.z);
}

void QVector3Field::OnXChanged(float)
{
	emit ValueChanged(value());
}

void QVector3Field::OnYChanged(float)
{
	emit ValueChanged(value());
}

void QVector3Field::OnZChanged(float)
{
	emit ValueChanged(value());
}


