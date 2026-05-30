#include "ConsoleLogWindow.h"
#include "Console/LogModel.h"
#include "Console/LogItemStyle.h"
#include "Console/LogFilter.h"
#include <EngineAPI.h>

LuxonEditor::GUI::QT::ConsoleLogWindow::ConsoleLogWindow(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

    m_logModel = new LogModel();
    m_filter = new LogFilter();
    m_filter->setSourceModel(m_logModel);
    m_filter->SetMask(LuxonEngine::LogType::Log, ui.logFilterCheckBox->isChecked());
    m_filter->SetMask(LuxonEngine::LogType::Warning, ui.warningFilterCheckBox->isChecked());
    m_filter->SetMask(LuxonEngine::LogType::Error, ui.errorFilterCheckBox->isChecked());
    ui.logListView->setModel(m_filter);
    ui.logListView->setItemDelegate(new LogItemStyle());

    connect(ui.clearButton, &QPushButton::clicked, this, [this]() {
        LuxonEngine::Logger::ClearLogs();
        });
    connect(ui.logFilterCheckBox, &QCheckBox::checkStateChanged, [this](bool checked) {
        m_filter->SetMask(LuxonEngine::LogType::Log, checked);
        });
    connect(ui.warningFilterCheckBox, &QCheckBox::checkStateChanged, [this](bool checked) {
        m_filter->SetMask(LuxonEngine::LogType::Warning, checked);
        });
    connect(ui.errorFilterCheckBox, &QCheckBox::checkStateChanged, [this](bool checked) {
        m_filter->SetMask(LuxonEngine::LogType::Error, checked);
        });
}

LuxonEditor::GUI::QT::ConsoleLogWindow::~ConsoleLogWindow()
{}

