#pragma once
#include <QStyledItemDelegate>

namespace LuxonEditor::GUI::QT {
	class GameEntityItemDelegate : public QStyledItemDelegate
	{
		Q_OBJECT

	public:
		explicit GameEntityItemDelegate(QObject* parent = nullptr);
		~GameEntityItemDelegate();

		void paint(QPainter* painter, const QStyleOptionViewItem& option,
			const QModelIndex& index) const override;

		QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
			const QModelIndex& index) const override;

		void setEditorData(QWidget* editor, const QModelIndex& index) const override;

		void setModelData(QWidget* editor, QAbstractItemModel* model,
			const QModelIndex& index) const override;

		void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
			const QModelIndex& index) const override;
	};

}