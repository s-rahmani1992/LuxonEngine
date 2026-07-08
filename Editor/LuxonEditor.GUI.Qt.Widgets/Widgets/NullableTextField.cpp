#include "NullableTextField.h"
#include <qcheckbox.h>
#include <qnamespace.h>

QNullableTextField::QNullableTextField(QWidget* parent, bool isDesignerMode)
	: QWidget(parent), m_isDesignerMode(isDesignerMode)
{
	ui.setupUi(this);

	// runtime only
	if (m_isDesignerMode == false) {
		ui.inputText->setVisible(ui.labelBox->isChecked());
	}
	// runtime only

	connect(ui.inputText, &QLineEdit::textChanged, this, [this](const QString& text) {
		OnTextChanged(text);
		});
	connect(ui.labelBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
		ui.inputText->setVisible(state == Qt::CheckState::Checked);
		emit ValueChanged(HasValidValue());
		});
}

QNullableTextField::~QNullableTextField()
{}

QString QNullableTextField::labelText() const
{
	return ui.labelBox->text();
}

void QNullableTextField::setLabelText(const QString& t)
{
	ui.labelBox->setText(t);
}

QString QNullableTextField::placeHolder() const
{
	return ui.inputText->placeholderText();
}

void QNullableTextField::setPlaceHolder(const QString& t)
{
	ui.inputText->setPlaceholderText(t);
}

char* QNullableTextField::GetText()
{
	if(ui.labelBox->isChecked() == false) {
		return nullptr;
	}
	m_strValue = ui.inputText->text().toStdString();
	return m_strValue.data();
}

void QNullableTextField::OnTextChanged(const QString& text)
{
	if (m_validationFunc) {
		m_hasValidValue = m_validationFunc(text);
		if (!m_hasValidValue) {
			ui.inputText->setStyleSheet("border: 1px solid red;");
		}
		else {
			ui.inputText->setStyleSheet("border: none;");
		}

		emit ValueChanged(HasValidValue());
	}
}

