#include "QColorField.h"
#include <qpainter.h>

QColorField::QColorField(QWidget *parent, std::string fieldName)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.label->setText(QString::fromStdString(fieldName));
	layout()->setAlignment(ui.label, Qt::AlignTop | Qt::AlignLeft);
	layout()->setAlignment(ui.textureArea, Qt::AlignVCenter | Qt::AlignLeft);

	static_cast<QHBoxLayout*>(layout())->addStretch(1);

}

QColorField::~QColorField()
{}

void QColorField::SetColor(LuxonEngine::Color color)
{
	m_color = QColor::fromRgbF(color.GetColorArray()[0], color.GetColorArray()[1], color.GetColorArray()[2], color.GetColorArray()[3]);
	update();
}

void QColorField::paintEvent(QPaintEvent * event)
{
	QPainter p(this);
	p.fillRect(ui.textureArea->geometry(), m_color); // Fill with red
}

