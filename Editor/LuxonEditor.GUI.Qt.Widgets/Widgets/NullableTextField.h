#pragma once

#include <QWidget>
#include "ui_NullableTextField.h"

class QNullableTextField : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(QString labelText READ labelText WRITE setLabelText DESIGNABLE true)

	Q_PROPERTY(QString placeHolder READ placeHolder WRITE setPlaceHolder DESIGNABLE true)

public:
	QNullableTextField(QWidget* parent = nullptr, bool isDesignerMode = false);
	~QNullableTextField();
	QString labelText() const;
	void setLabelText(const QString& t);
	QString placeHolder() const;
	void setPlaceHolder(const QString& t);

	void Toggle(bool isOn) {
		ui.labelBox->setChecked(isOn);
	}
	QLineEdit* InputText() const { return ui.inputText; }
	char* GetText();
	void RegisterValidationFunction(std::function<bool(const QString&)> func) {
		m_validationFunc = func;
		OnTextChanged(ui.inputText->text());
	}

	bool HasValidValue() const { return !ui.labelBox->isChecked() || m_hasValidValue; }

Q_SIGNALS:
	void ValueChanged(bool isValid);

private:
	void OnTextChanged(const QString& text);

	Ui::NullableTextFieldClass ui;
	bool m_isDesignerMode;
	std::function<bool(const QString&)> m_validationFunc;
	bool m_hasValidValue = true;
	std::string m_strValue;
};
