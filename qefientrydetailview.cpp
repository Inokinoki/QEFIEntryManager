#include "qefientrydetailview.h"
#include "helpers.h"

QEFIEntryDetailBriefView::QEFIEntryDetailBriefView(
    QEFIEntry &entry, QWidget *parent)
    : QWidget(parent), m_entry(entry)
{
    m_briefLayout = new QFormLayout(this);

    m_briefLayout->addRow("ID:",
        new QLabel(QString::asprintf("Boot%04X ", entry.id())));
    m_briefLayout->addRow("Name:", new QLabel(entry.name()));

    QEFILoadOption *loadOption = entry.loadOption();
    if (loadOption) {
        auto dpList = loadOption->devicePathList();
        m_briefLayout->addRow("Device Path instance:",
            new QLabel(QString::number(dpList.size())));
        // Add a tab to display each DP
        for (int i = 0; i < dpList.size(); i++) {
            // Display type name
            m_briefLayout->addRow(QString::asprintf("Device Path %d type:", i + 1),
                new QLabel(
                    convert_device_path_type_to_name(dpList[i]->type()) + " " +
                    convert_device_path_subtype_to_name(dpList[i]->type(),
                        dpList[i]->subType())));
        }
    } else {
        m_briefLayout->addRow("Device Path instance:",
            new QLabel(QString::number(0)));
    }
}

QEFIEntryDetailBriefView::~QEFIEntryDetailBriefView()
{
    if (m_briefLayout != nullptr) delete m_briefLayout;
    m_briefLayout = nullptr;
}


QEFIEntryDetailView::QEFIEntryDetailView(QEFIEntry &entry, QWidget *parent)
    : QWidget(parent), m_entry(entry)
{
    m_topLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    m_tab = new QTabWidget(this);
    m_tab->addTab(new QEFIEntryDetailBriefView(entry, m_tab),
        QStringLiteral("Brief"));

    QEFILoadOption *loadOption = entry.loadOption();
    if (loadOption) {
        auto dpList = loadOption->devicePathList();
        // Add a tab to display each DP
        for (int i = 0; i < dpList.size(); i++) {
            m_tab->addTab(new QEFIEntryDPDetailView(dpList[i].get(), m_tab),
                QString::asprintf("DP %d", i + 1));
        }
    }
    m_topLevelLayout->addWidget(m_tab);
    setLayout(m_topLevelLayout);
}

QEFIEntryDetailView::~QEFIEntryDetailView()
{
    if (m_topLevelLayout != nullptr) delete m_topLevelLayout;
    m_topLevelLayout = nullptr;

    // Has ownership been passed to layout?
    if (m_tab != nullptr) delete m_tab;
    m_tab = nullptr;
}
