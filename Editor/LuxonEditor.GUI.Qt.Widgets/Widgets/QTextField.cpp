#include "QTextField.h"

QTextField::QTextField(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.inputText, &QLineEdit::textChanged, this, [this](const QString& text) {
		OnTextChanged(text);
		});
}

QTextField::~QTextField()
{}

QString QTextField::labelText() const
{
	return ui.label->text();
}

void QTextField::setLabelText(const QString& t)
{
	ui.label->setText(t);
}

QString QTextField::placeHolder() const
{
	return ui.inputText->placeholderText();
}

void QTextField::setPlaceHolder(const QString& t)
{
	ui.inputText->setPlaceholderText(t);
}

char* QTextField::GetText()
{
	m_strValue = ui.inputText->text().toStdString();
	return m_strValue.empty() ? nullptr : m_strValue.data();
}

void QTextField::OnTextChanged(const QString& text)
{
	if (m_validationFunc) {
		m_hasValidValue = m_validationFunc(text);
		if (!m_hasValidValue) {
			ui.inputText->setStyleSheet("border: 1px solid red;");
		}
		else {
			ui.inputText->setStyleSheet("border: none;");
		}

		emit ValueChanged(m_hasValidValue);
	}
}
