#pragma once

#include <QWidget>
#include "ui_QTextureField.h"
#include <EngineAPI.h>

class QTextureField : public QWidget
{
	Q_OBJECT

public:
	QTextureField(QWidget *parent = nullptr, std::string fieldName = "");
	~QTextureField();
	void SetTexture(ref<LuxonEngine::Texture2D> texture);

protected:
	//virtual void paintEvent(QPaintEvent* event) override;
	virtual void paintEvent(QPaintEvent* event) override;
private:
	Ui::QTextureFieldClass ui;
	QImage m_textureImage;
};

