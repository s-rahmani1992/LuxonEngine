#pragma once

#include <QWidget>
#include "ui_QColorField.h"
#include <EngineAPI.h>

class QColorField : public QWidget
{
	Q_OBJECT

public:
	QColorField(QWidget* parent = nullptr, std::string fieldName = "");
	~QColorField();
	void SetColor(LuxonEngine::Color color);

Q_SIGNALS:
	void ValueChanged(LuxonEngine::Color newColor);

protected:
	virtual void paintEvent(QPaintEvent* event) override;
	virtual void mousePressEvent(QMouseEvent* event) override;

private:
	//bool eventFilter(QObject* obj, QEvent* event);

	Ui::QColorFieldClass ui;
	QColor m_color;
};
