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

	char* GetText() const;

private:
	Ui::NullableTextFieldClass ui;
	bool m_isDesignerMode;
};
