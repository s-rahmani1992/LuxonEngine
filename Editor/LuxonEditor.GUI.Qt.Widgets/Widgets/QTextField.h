#pragma once

#include <QWidget>
#include "ui_QTextField.h"

class QTextField : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(QString labelText READ labelText WRITE setLabelText DESIGNABLE true)

	Q_PROPERTY(QString placeHolder READ placeHolder WRITE setPlaceHolder DESIGNABLE true)

public:
	QTextField(QWidget* parent = nullptr, std::string text = "");
	~QTextField();
	QString labelText() const;
	void setLabelText(const QString& t);
	QString placeHolder() const;
	void setPlaceHolder(const QString& t);

	QLineEdit* InputText() const { return ui.inputText; }
	void Validate();

	char* GetText();
	std::string GetStdString() const { return ui.inputText->text().toStdString(); }

	void RegisterValidationFunction(std::function<bool(const QString&)> func) {
		m_validationFunc = func;
		OnTextChanged(ui.inputText->text());
	}

	bool HasValidValue() const { return m_hasValidValue; }

Q_SIGNALS:
	void ValueChanged(bool isValid);

private:
	void OnTextChanged(const QString& text);

	Ui::QTextFieldClass ui;

	std::function<bool(const QString&)> m_validationFunc;

	bool m_hasValidValue = true;
	std::string m_strValue;
};

