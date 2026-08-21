#include "PathItemStyle.h"
#include <qfilesystemmodel.h>
#include <qpainter.h>
#include <QLineEdit>
#include <LuxonEditorAPI.h>
#include <Core/SerializationStream.h>

LuxonEditor::GUI::QT::PathItemStyle::PathItemStyle(PathFilter* proxy, QObject* parent)
    : QStyledItemDelegate(parent)
    , m_proxy(proxy)
    , m_fileModel(qobject_cast<QFileSystemModel*>(proxy->sourceModel()))
{
}

void LuxonEditor::GUI::QT::PathItemStyle::paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const
{
    QModelIndex srcIdx = m_proxy->mapToSource(idx);
    p->save();

    QString path = m_fileModel->fileName(srcIdx);
    auto fileInfo = m_fileModel->fileInfo(srcIdx);
	auto iconRect = QRect(opt.rect.left(), opt.rect.top(), opt.rect.width(), opt.rect.width());
    if (opt.state & QStyle::State_Selected) {
        p->fillRect(opt.rect, QColor("#1133ee"));
        p->setPen(Qt::white);
    }
    else if (opt.state & QStyle::State_MouseOver) {
        p->fillRect(opt.rect, QColor("#3366bb")); // hover color
    }

    auto textRect = QRect(
        opt.rect.left(),
        iconRect.bottom(),
        opt.rect.width(),
        opt.rect.height() - iconRect.height()
    );

    p->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWrapAnywhere, fileInfo.completeBaseName());


    auto ext = fileInfo.suffix().toLower();
    if(fileInfo.isDir())
        p->drawImage(iconRect, m_folderIcon);
    else if(ext == "hlsl" || ext == "hlsli")
		p->drawImage(iconRect, m_hlslIcon);
    else if(ext == "png" || ext == "jpeg" || ext == "jpg") {
        std::string filePath = fileInfo.absoluteFilePath().toStdString();
		SerializationStream stream;
		stream.LoadFromFile(filePath + ".json");
		auto guid = stream.GetGuid("uuid");
		auto texture = GetAssetManager()->GetTexture(guid);
		int iterationCount = 0;
		while (texture == nullptr) { //TODO: This is a hacky solution, we should have a better way to handle this to render after file import
            texture = GetAssetManager()->GetTexture(guid);
            iterationCount++;
			if (iterationCount > 100)
                return;
		}

        QImage::Format format = texture->GetFormat() == LuxonEngine::TextureFormat::RGBA32 ? QImage::Format_RGBA8888 : QImage::Format_ARGB32;
        
        p->drawImage(iconRect, QImage(
            texture->GetData(),
            texture->GetWidth(),
            texture->GetHeight(),
            format
        ));
    }
    else if (ext == "fbx")
        p->drawImage(iconRect, m_modelIcon);
    else if (ext == "lmat")
		p->drawImage(iconRect, m_materialIcon);
	else if (ext == "lscene")
		p->drawImage(iconRect, m_sceneIcon);
    else
        p->drawImage(iconRect, m_fileIcon);

    p->restore();
	
}

QWidget* LuxonEditor::GUI::QT::PathItemStyle::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QLineEdit* edit = new QLineEdit(parent);
    edit->setFrame(false);
    edit->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    return edit;
}

void LuxonEditor::GUI::QT::PathItemStyle::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    QModelIndex srcIdx = m_proxy->mapToSource(index);
    QString name = m_fileModel->fileInfo(srcIdx).completeBaseName();

    QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
    edit->setText(name);
    edit->selectAll();
}

void LuxonEditor::GUI::QT::PathItemStyle::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
    QString newName = edit->text().trimmed();
    if (newName.isEmpty())
        return; // ignore empty rename

    // Get source index (because you use a proxy)
    QModelIndex srcIdx = m_proxy->mapToSource(index);

    QFileInfo info = m_fileModel->fileInfo(srcIdx);
    QString oldPath = info.absoluteFilePath();

	AssetRegistry_RenamePath(GetAssetManager(), oldPath.toStdString(), newName.toStdString());
}

void LuxonEditor::GUI::QT::PathItemStyle::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
    QRect iconRect(opt.rect.left(), opt.rect.top(), opt.rect.width(), opt.rect.width());

    QRect textRect(
        opt.rect.left(),
        iconRect.bottom(),
        opt.rect.width(),
        opt.rect.height() - iconRect.height()
    );

    editor->setGeometry(textRect);
}

QImage LuxonEditor::GUI::QT::PathItemStyle::GetIconForIndex(const QModelIndex& idx) const
{
    auto fileInfo = m_fileModel->fileInfo(idx);
    auto ext = fileInfo.suffix().toLower();
    if (fileInfo.isDir())
        return m_folderIcon;
    else if(ext == "hlsl" || ext == "hlsli")
		return m_hlslIcon;
    else if (ext == "png" || ext == "jpeg" || ext == "jpg") {
        std::string filePath = fileInfo.absoluteFilePath().toStdString();
        SerializationStream stream;
        stream.LoadFromFile(filePath + ".json");
        auto guid = stream.GetGuid("uuid");
        auto texture = GetAssetManager()->GetTexture(guid);
        QImage::Format format = texture->GetFormat() == LuxonEngine::TextureFormat::RGBA32 ? QImage::Format_RGBA8888 : QImage::Format_ARGB32;

        return QImage(
            texture->GetData(),
            texture->GetWidth(),
            texture->GetHeight(),
            format
        );
    }
    else if(ext == "fbx" || ext == "obj")
		return m_modelIcon;
    else if(ext == "lmat")
		return m_materialIcon;
    else
		return m_fileIcon;
}
