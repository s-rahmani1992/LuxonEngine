#include "NullableTextField.h"
#include <qcheckbox.h>
#include <qnamespace.h>

QNullableTextField::QNullableTextField(QWidget* parent, bool isDesignerMode)
	: QWidget(parent), m_isDesignerMode(isDesignerMode)
{
	ui.setupUi(this);

	// runtime only
	if(m_isDesignerMode == false)
		ui.inputText->setVisible(ui.labelBox->isChecked());
	// runtime only

	connect(ui.labelBox, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
		ui.inputText->setVisible(state == Qt::CheckState::Checked);
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

char* QNullableTextField::GetText() const
{
	auto text = ui.inputText->text();
	return text.isEmpty() ? nullptr : text.toStdString().data();
}

