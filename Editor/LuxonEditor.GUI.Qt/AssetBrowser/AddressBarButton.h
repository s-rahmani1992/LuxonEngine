#pragma once

#include <QToolButton>

namespace LuxonEditor::GUI::QT {

	class AddressBarButton : public QToolButton
	{
		Q_OBJECT

	public:
		explicit AddressBarButton(QWidget* parent = nullptr);

		// assign the path this button represents
		void setPath(const QString& path) { m_path = path; }
		QString path() const { return m_path; }

	protected:
		bool event(QEvent* event) override;
		void dragEnterEvent(QDragEnterEvent* event) override;
		void dragMoveEvent(QDragMoveEvent* event) override;
		void dragLeaveEvent(QDragLeaveEvent* event) override;
		void dropEvent(QDropEvent* event) override;

	private:
		QString m_path;

		QPalette m_originalPalette;
		bool m_hasOriginalPalette = false;
	};

}