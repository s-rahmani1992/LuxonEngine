#include "GameEntityModel.h"
#include <boost/uuid/uuid_io.hpp>

namespace LuxonEditor::GUI::QT {
	GameEntityModel::GameEntityModel(EngineSceneManager* sceneManager, QObject* parent)
		: QAbstractListModel(parent), m_sceneManager(sceneManager)
	{
		if (!m_sceneManager)
			return;

		m_callbackId = m_sceneManager->RegisterEntityListChangedCallback(
			[this]() {
				OnEntityListChanged();
			});
	}

	GameEntityModel::~GameEntityModel()
	{
		if (m_sceneManager)
			m_sceneManager->UnregisterEntityListChangedCallback(m_callbackId);
	}

	int GameEntityModel::rowCount(const QModelIndex& parent) const
	{
		if (parent.isValid())
			return 0;

		if (!m_sceneManager)
			return 0;

		return static_cast<int>(m_sceneManager->GetEntityList().size());
	}

	QVariant GameEntityModel::data(const QModelIndex& index, int role) const
	{
		if (!m_sceneManager || !index.isValid())
			return QVariant();

		const auto& entityList = m_sceneManager->GetEntityList();

		if (index.row() >= static_cast<int>(entityList.size()))
			return QVariant();

		const EngineSceneManager::EntityEntry& entry = entityList.at(index.row());

		switch (role)
		{
		case Qt::DisplayRole:
		case Qt::EditRole:
		case NameRole:
			return QString::fromStdString(entry.entity->GetName());
		case UUIDRole:
			return QString::fromStdString(boost::uuids::to_string(entry.uuid));
		default:
			return QVariant();
		}
	}

	bool GameEntityModel::setData(const QModelIndex& index, const QVariant& value, int role)
	{
		if (!m_sceneManager || !index.isValid() || role != Qt::EditRole)
			return false;

		const auto& entityList = m_sceneManager->GetEntityList();

		if (index.row() >= static_cast<int>(entityList.size()))
			return false;

		const std::string newName = value.toString().trimmed().toStdString();
		if (newName.empty())
			return false;

		entityList.at(index.row()).entity->SetName(newName);
		emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole, NameRole });
		return true;
	}

	Qt::ItemFlags GameEntityModel::flags(const QModelIndex& index) const
	{
		if (!index.isValid())
			return Qt::NoItemFlags;

		return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
	}

	QHash<int, QByteArray> GameEntityModel::roleNames() const
	{
		QHash<int, QByteArray> roles;
		roles[NameRole] = "name";
		roles[UUIDRole] = "uuid";
		return roles;
	}

	void GameEntityModel::OnEntityListChanged()
	{
		beginResetModel();
		endResetModel();
	}

}