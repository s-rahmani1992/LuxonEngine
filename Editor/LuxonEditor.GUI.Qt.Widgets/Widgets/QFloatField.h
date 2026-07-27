#pragma once

#include <QWidget>
#include <QDoubleValidator>
#include <QPoint>
#include "ui_QFloatField.h"

class QFloatField : public QWidget
{
	Q_OBJECT

		Q_PROPERTY(QString labelText READ labelText WRITE setLabelText DESIGNABLE true)
		Q_PROPERTY(double value READ value WRITE setValue DESIGNABLE true)

public:
	QFloatField(QWidget* parent = nullptr, bool draggable = true);
	~QFloatField();

	QString labelText() const;
	void setLabelText(const QString& t);

	double value() const;
	void setValue(double v);

	QLineEdit* InputText() const { return ui.inputText; }

Q_SIGNALS:
	void ValueChanged(float newValue);

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	void OnTextChanged(const QString& text);

	Ui::FloatFieldClass ui;
	QDoubleValidator* m_validator;
	double m_value = 0.0;

	bool m_draggable = false;
	bool m_dragging = false;
	QPoint m_dragStartPos;
	double m_dragStartValue = 0.0;
};