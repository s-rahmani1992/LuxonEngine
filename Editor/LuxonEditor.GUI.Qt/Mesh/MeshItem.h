#pragma once

#include <QWidget>
#include "ui_MeshItem.h"
#include <Core/SerializationStream.h>

namespace LuxonEngine {
	class SerializationStream;
}

namespace LuxonEditor::GUI::QT {
	class MeshItem : public QWidget
	{
		Q_OBJECT

	public:
		MeshItem(QWidget* parent = nullptr, LuxonEngine::SerializationStream* stream = nullptr);
		~MeshItem();
		void Revert();
		void ApplyChanges() {m_originalStream = m_currentStream; }
		LuxonEngine::SerializationStream* GetCurrentStream() { return &m_currentStream; }
	protected:
		void paintEvent(QPaintEvent* event) override;

	private:
		Ui::MeshItemClass ui;
		LuxonEngine::SerializationStream m_currentStream;
		LuxonEngine::SerializationStream m_originalStream;
	};
}
