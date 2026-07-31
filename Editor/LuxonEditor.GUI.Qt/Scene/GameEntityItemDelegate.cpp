#include "GameEntityItemDelegate.h"
#include "GameEntityModel.h"
#include <QLineEdit>
#include <QPainter>
#include <QApplication>

namespace LuxonEditor::GUI::QT {

	GameEntityItemDelegate::GameEntityItemDelegate(QObject* parent)
		: QStyledItemDelegate(parent)
	{
	}

	GameEntityItemDelegate::~GameEntityItemDelegate()
	{
	}

	void GameEntityItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
		const QModelIndex& index) const
	{
		

		QStyleOptionViewItem opt = option;
		initStyleOption(&opt, index);

		// Draw standard background (selection highlight etc.)
		QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
		//style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
		painter->save();


		if (opt.state & QStyle::State_Selected) {
			painter->fillRect(opt.rect, QColor("#1133ee"));
			painter->setPen(Qt::white);
		}
		else if (opt.state & QStyle::State_MouseOver) {
			painter->fillRect(opt.rect, QColor("#3366bb")); // hover color
		}

		// Draw entity name
		const QString name = index.data(Qt::DisplayRole).toString();
		const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
		painter->setFont(opt.font);
		painter->setPen(opt.palette.text().color());
		painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, name);
		painter->restore();
	}

	QWidget* GameEntityItemDelegate::createEditor(QWidget* parent,
		const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const
	{
		QLineEdit* editor = new QLineEdit(parent);
		editor->setFrame(false);
		return editor;
	}

	void GameEntityItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
	{
		QLineEdit* lineEdit = static_cast<QLineEdit*>(editor);
		lineEdit->setText(index.data(Qt::EditRole).toString());
		lineEdit->selectAll();
	}

	void GameEntityItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
		const QModelIndex& index) const
	{
		QLineEdit* lineEdit = static_cast<QLineEdit*>(editor);
		model->setData(index, lineEdit->text(), Qt::EditRole);
	}

	void GameEntityItemDelegate::updateEditorGeometry(QWidget* editor,
		const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const
	{
		editor->setGeometry(option.rect);
	}
}