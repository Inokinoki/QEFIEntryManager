#include "qefientrydetailview.h"

QEFIEntryDetailView::QEFIEntryDetailView(QEFIEntry &entry, QWidget *parent)
    : QWidget(parent), m_entry(entry)
{
    m_topLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    m_briefLayout = new QFormLayout(this);
    m_topLevelLayout->addLayout(m_briefLayout, 1);

    m_briefLayout->addRow("ID:",
        new QLabel(QString::asprintf("Boot%04X ", entry.id())));
    m_briefLayout->addRow("Name:", new QLabel(entry.name()));

    QEFILoadOption *loadOption = entry.loadOption();
    if (loadOption) {
        auto dpList = loadOption->devicePathList();
        m_briefLayout->addRow("Device Path instance:",
            new QLabel(QString::number(dpList.size())));
        // TODO: Add a tab to display each DP
        for (int i = 0; i < dpList.size(); i++) {
            // TODO: Replace by name
            m_briefLayout->addRow(QString::asprintf("Device Path %d type:", i + 1),
                new QLabel(QString::asprintf("%02X %02X",
                    dpList[i]->type(), dpList[i]->subType())));
        }
    } else {
        m_briefLayout->addRow("Device Path instance:",
            new QLabel(QString::number(0)));
    }
}

QEFIEntryDetailView::~QEFIEntryDetailView()
{
    // TODO: Use smart ptr
    if (m_topLevelLayout != nullptr) {
        delete m_topLevelLayout;
        m_topLevelLayout = nullptr;
    }
}
