#pragma once
#include <QAbstractListModel>
#include <EngineAPI.h>
#include <Core/EngineSceneManager.h>

namespace LuxonEditor::GUI::QT {
	class GameEntityModel : public QAbstractListModel
	{
		Q_OBJECT

	public:
		enum Roles {
			NameRole = Qt::UserRole + 1,
			UUIDRole
		};

		explicit GameEntityModel(EngineSceneManager* sceneManager, QObject* parent = nullptr);
		~GameEntityModel();

		int rowCount(const QModelIndex& parent = QModelIndex()) const override;
		QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
		bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
		Qt::ItemFlags flags(const QModelIndex& index) const override;
		QHash<int, QByteArray> roleNames() const override;

	private:
		void OnEntityListChanged();

		EngineSceneManager* m_sceneManager = nullptr;
		size_t m_callbackId = 0;
	};
}