#include "QFloatField.h"

QFloatField::QFloatField(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	m_validator = new QDoubleValidator(this);
	m_validator->setNotation(QDoubleValidator::StandardNotation);
	ui.inputText->setValidator(m_validator);

	connect(ui.inputText, &QLineEdit::textChanged, this, [this](const QString& text) {
		OnTextChanged(text);
	});
}

QFloatField::~QFloatField()
{}

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

