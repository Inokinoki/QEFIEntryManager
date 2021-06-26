#include "qefientryview.h"

#include "qefientrystaticlist.h"

QEFIEntryView::QEFIEntryView(QWidget *parent)
    : QWidget(parent)
{
    // Add list view and buttons
    m_topLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);

    m_entries = new QListWidget(this);
    m_entryItems = QEFIEntryStaticList::instance()->entries();
    m_order = QEFIEntryStaticList::instance()->order();
    for (int i = 0; i < m_order.size(); i++) {
        if (m_entryItems.contains(m_order[i])) {
            QEFIEntry &entry = m_entryItems[m_order[i]];
            m_entries->addItem(QString::asprintf("[%04X] ", entry.id()) + entry.name());
        }
    }
    m_topLevelLayout->addWidget(m_entries, 3);

    m_buttonLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    m_topLevelLayout->addLayout(m_buttonLayout, 1);

    m_moveUpEntryButton = new QPushButton(QStringLiteral("Move up"), this);
    m_moveDownEntryButton = new QPushButton(QStringLiteral("Move down"), this);
    m_setCurrentButton = new QPushButton(QStringLiteral("Make default"), this);
    m_buttonLayout->addWidget(m_moveUpEntryButton);
    m_buttonLayout->addWidget(m_moveDownEntryButton);
    m_buttonLayout->addWidget(m_setCurrentButton);

    m_buttonLayout->addStretch(1);

    m_saveButton = new QPushButton(QStringLiteral("Save"), this);
    m_resetButton = new QPushButton(QStringLiteral("Reset"), this);
    m_buttonLayout->addWidget(m_saveButton);
    m_buttonLayout->addWidget(m_resetButton);

    this->setLayout(m_topLevelLayout);
}

QEFIEntryView::~QEFIEntryView()
{
    // TODO: Use smart ptr
    if (m_topLevelLayout != nullptr) {
        delete m_topLevelLayout;
        m_topLevelLayout = nullptr;
    }

    if (m_entries != nullptr) delete m_entries;
    m_entries = nullptr;

    // Seems that we have no ownership on it
//    if (m_buttonLayout != nullptr) delete m_buttonLayout;
//    m_buttonLayout = nullptr;

    if (m_moveUpEntryButton != nullptr) delete m_moveUpEntryButton;
    if (m_moveDownEntryButton != nullptr) delete m_moveDownEntryButton;
    if (m_setCurrentButton != nullptr) delete m_setCurrentButton;
    if (m_saveButton != nullptr) delete m_saveButton;
    if (m_resetButton != nullptr) delete m_resetButton;
}

