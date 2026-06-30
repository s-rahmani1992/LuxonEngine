
#pragma once
#include <QtUiPlugin/QDesignerCustomWidgetCollectionInterface>
#include <QList>

class LuxonEditorWidgetsPlugin : public QObject, public QDesignerCustomWidgetCollectionInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QDesignerCustomWidgetCollectionInterface" FILE "designerplugins.json")
	Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

public:
	explicit LuxonEditorWidgetsPlugin(QObject* parent = nullptr);
	QList<QDesignerCustomWidgetInterface*> customWidgets() const override;

private:
	QList<QDesignerCustomWidgetInterface*> m_widgets;
};
