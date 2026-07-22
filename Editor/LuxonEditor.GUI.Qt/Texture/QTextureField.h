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

Q_SIGNALS:
	void ValueChanged(ref<LuxonEngine::Texture2D> newTexture);

protected:
	//virtual void paintEvent(QPaintEvent* event) override;
	virtual void paintEvent(QPaintEvent* event) override;
private:
	Ui::QTextureFieldClass ui;
	ref<LuxonEngine::Texture2D> m_texture;
	QImage m_textureImage;
};

