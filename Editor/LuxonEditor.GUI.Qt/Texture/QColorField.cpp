#include "QColorField.h"
#include <qpainter.h>
#include <QColorDialog>
#include <QMouseEvent>

QColorField::QColorField(QWidget* parent, std::string fieldName)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.label->setText(QString::fromStdString(fieldName));
	layout()->setAlignment(ui.label, Qt::AlignTop | Qt::AlignLeft);
	layout()->setAlignment(ui.textureArea, Qt::AlignVCenter | Qt::AlignLeft);

	static_cast<QHBoxLayout*>(layout())->addStretch(1);
}

QColorField::~QColorField()
{
}

void QColorField::SetColor(LuxonEngine::Color color)
{
	m_color = QColor::fromRgbF(color.GetColorArray()[0], color.GetColorArray()[1], color.GetColorArray()[2], color.GetColorArray()[3]);
	update();
}

void QColorField::paintEvent(QPaintEvent* event)
{
	QPainter p(this);
	p.fillRect(ui.textureArea->geometry(), m_color);
}

void QColorField::mousePressEvent(QMouseEvent* event)
{
	if (ui.textureArea->geometry().contains(event->pos()))
	{
		QColor selected = QColorDialog::getColor(m_color, this, "Select Color", QColorDialog::ShowAlphaChannel);
		if (selected.isValid())
		{
			m_color = selected;
			update();

			LuxonEngine::Color newColor(
				static_cast<float>(m_color.redF()),
				static_cast<float>(m_color.greenF()),
				static_cast<float>(m_color.blueF()),
				static_cast<float>(m_color.alphaF())
			);
			emit ValueChanged(newColor);
		}
	}

	QWidget::mousePressEvent(event);
}