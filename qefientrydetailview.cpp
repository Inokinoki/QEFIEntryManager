#include "qefientrydetailview.h"
#include "helpers.h"

QEFIEntryDetailBriefView::QEFIEntryDetailBriefView(
    QEFIEntry &entry, QWidget *parent)
    : QWidget(parent), m_entry(entry)
{
    m_briefLayout = new QFormLayout(this);

    m_briefLayout->addRow("ID:",
        new QLabel(QStringLiteral("Boot%1 ").arg(entry.id(), 4, 16, QLatin1Char('0'))));
    m_briefLayout->addRow("Name:", new QLabel(entry.name()));

    QEFILoadOption *loadOption = entry.loadOption();
    if (loadOption) {
        auto dpList = loadOption->devicePathList();
        m_briefLayout->addRow(tr("Device Path instance:"),
            new QLabel(QString::number(dpList.size())));
        // Add a tab to display each DP
        for (int i = 0; i < dpList.size(); i++) {
            // Display type name
            m_briefLayout->addRow(tr("Device Path %1 type:").arg(i + 1),
                new QLabel(
                    convert_device_path_type_to_name(dpList[i]->type()) + " " +
                    convert_device_path_subtype_to_name(dpList[i]->type(),
                        dpList[i]->subType())));
        }
    } else {
        m_briefLayout->addRow("Device Path instance:",
            new QLabel(QString::number(0)));
    }
    m_briefLayout->addRow("Optional data size:",
        new QLabel(QString::number(loadOption->optionalData().size())));
    if (loadOption->optionalData().size() < DISPLAY_DATA_LIMIT) {
        m_briefLayout->addRow("Optional data:",
            new QLabel(loadOption->optionalData()));
    } else {
        m_briefLayout->addRow("Optional data:", new QLabel(
            QString(loadOption->optionalData()
                .left(DISPLAY_DATA_LIMIT).toHex()) + "..."));
    }
}

QEFIEntryDetailBriefView::~QEFIEntryDetailBriefView()
{
    if (m_briefLayout != nullptr) m_briefLayout->deleteLater();
    m_briefLayout = nullptr;
}


QEFIEntryDetailView::QEFIEntryDetailView(QEFIEntry &entry, QWidget *parent)
    : QWidget(parent), m_entry(entry)
{
    m_topLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    m_tab = new QTabWidget(this);
    m_tab->addTab(new QEFIEntryDetailBriefView(entry, m_tab),
        tr("Brief"));

    QEFILoadOption *loadOption = entry.loadOption();
    if (loadOption) {
        auto dpList = loadOption->devicePathList();
        // Add a tab to display each DP
        for (int i = 0; i < dpList.size(); i++) {
            m_tab->addTab(new QEFIEntryDPDetailView(dpList[i].get(), m_tab),
                QStringLiteral("DP %1").arg(i + 1));
        }
    }
    m_topLevelLayout->addWidget(m_tab);
    setLayout(m_topLevelLayout);
}

QEFIEntryDetailView::~QEFIEntryDetailView()
{
    if (m_topLevelLayout != nullptr) m_topLevelLayout->deleteLater();
    m_topLevelLayout = nullptr;

    if (m_tab != nullptr) m_tab->deleteLater();
    m_tab = nullptr;
}
