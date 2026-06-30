#pragma once

#include <QWidget>
#include "ui_QTextField.h"

class QTextField : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(QString labelText READ labelText WRITE setLabelText DESIGNABLE true)

	Q_PROPERTY(QString placeHolder READ placeHolder WRITE setPlaceHolder DESIGNABLE true)

public:
	QTextField(QWidget* parent = nullptr);
	~QTextField();
	QString labelText() const;
	void setLabelText(const QString& t);
	QString placeHolder() const;
	void setPlaceHolder(const QString& t);

	char* GetText() const;

private:
	Ui::QTextFieldClass ui;
};

