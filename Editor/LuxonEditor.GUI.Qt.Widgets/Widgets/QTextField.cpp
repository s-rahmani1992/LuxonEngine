#include "QTextField.h"

QTextField::QTextField(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
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

char* QTextField::GetText() const
{
	auto text = ui.inputText->text();
	return text.isEmpty() ? nullptr : text.toStdString().data();
}
