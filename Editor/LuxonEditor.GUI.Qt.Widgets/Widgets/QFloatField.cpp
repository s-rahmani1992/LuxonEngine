#include "QFloatField.h"
#include <QMouseEvent>
#include <QApplication>
#include <QCursor>

QFloatField::QFloatField(QWidget* parent, bool draggable)
	: QWidget(parent), m_draggable(draggable)
{
	ui.setupUi(this);

	layout()->setAlignment(ui.label, Qt::AlignLeft);

	m_validator = new QDoubleValidator(this);
	m_validator->setNotation(QDoubleValidator::StandardNotation);
	ui.inputText->setValidator(m_validator);

	connect(ui.inputText, &QLineEdit::textChanged, this, [this](const QString& text) {
		OnTextChanged(text);
		});

	if (m_draggable)
	{
		ui.label->setCursor(Qt::SizeHorCursor);
		ui.label->installEventFilter(this);
	}
}

QFloatField::~QFloatField()
{
}

QString QFloatField::labelText() const
{
	return ui.label->text();
}

void QFloatField::setLabelText(const QString& t)
{
	ui.label->setText(t);
}

double QFloatField::value() const
{
	return m_value;
}

void QFloatField::setValue(double v)
{
	m_value = v;
	ui.inputText->setText(QString::number(v));
}

bool QFloatField::eventFilter(QObject* watched, QEvent* event)
{
	if (m_draggable && watched == ui.label)
	{
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

		if (event->type() == QEvent::MouseButtonPress && mouseEvent->button() == Qt::LeftButton)
		{
			m_dragging = true;
			m_dragStartPos = mouseEvent->globalPosition().toPoint();
			m_dragStartValue = m_value;
			QApplication::setOverrideCursor(Qt::BlankCursor);
			return true;
		}
		else if (event->type() == QEvent::MouseMove && m_dragging)
		{
			int delta = mouseEvent->globalPosition().toPoint().x() - m_dragStartPos.x();
			double newValue = m_dragStartValue + delta * 0.1;
			setValue(newValue);
			emit ValueChanged((float)m_value);
			return true;
		}
		else if (event->type() == QEvent::MouseButtonRelease && mouseEvent->button() == Qt::LeftButton)
		{
			m_dragging = false;
			QApplication::restoreOverrideCursor();
			return true;
		}
	}

	return QWidget::eventFilter(watched, event);
}

void QFloatField::OnTextChanged(const QString& text)
{
	bool ok = false;
	double parsed = text.toDouble(&ok);
	if (ok) {
		m_value = parsed;
		ui.inputText->setStyleSheet("border: none;");
		emit ValueChanged((float)m_value);
	}
	else if (text.isEmpty()) {
		ui.inputText->setStyleSheet("border: none;");
	}
	else {
		ui.inputText->setStyleSheet("border: 1px solid red;");
	}
}