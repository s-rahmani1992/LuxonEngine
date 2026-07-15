#pragma once

#include <QWidget>
#include "ui_QVector3Field.h"
#include <Core/Vector3.h>

class QVector3Field : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(QString labelText READ labelText WRITE setLabelText DESIGNABLE true)
	Q_PROPERTY(double valueX READ valueX WRITE setValueX DESIGNABLE true)
	Q_PROPERTY(double valueY READ valueY WRITE setValueY DESIGNABLE true)
	Q_PROPERTY(double valueZ READ valueZ WRITE setValueZ DESIGNABLE true)

public:
	QVector3Field(QWidget *parent = nullptr);
	~QVector3Field();

	QString labelText() const;
	void setLabelText(const QString& t);

	double valueX() const;
	void setValueX(double v);

	double valueY() const;
	void setValueY(double v);

	double valueZ() const;
	void setValueZ(double v);

	LuxonEngine::Vector3 value() const;
	void setValue(const LuxonEngine::Vector3& v);

Q_SIGNALS:
	void ValueChanged(const LuxonEngine::Vector3& newValue);

private:
	void OnXChanged(float v);
	void OnYChanged(float v);
	void OnZChanged(float v);

	Ui::QVector3FieldClass ui;
};

