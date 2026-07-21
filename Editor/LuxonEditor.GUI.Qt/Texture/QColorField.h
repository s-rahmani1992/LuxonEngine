#pragma once

#include <QWidget>
#include "ui_QColorField.h"
#include <EngineAPI.h>

class QColorField : public QWidget
{
	Q_OBJECT

public:
	QColorField(QWidget *parent = nullptr, std::string fieldName = "");
	~QColorField();
	void SetColor(LuxonEngine::Color color);
protected:
	virtual void paintEvent(QPaintEvent* event) override;
private:
	Ui::QColorFieldClass ui;
	QColor m_color;
};

