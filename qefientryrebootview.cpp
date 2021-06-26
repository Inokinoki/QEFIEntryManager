#include "qefientryrebootview.h"

QEFIEntryRebootView::QEFIEntryRebootView(QWidget *parent)
    : QWidget(parent)
{
    // TODO: Add list view and buttons
    m_topLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);

    m_entries = new QListWidget(this);
    m_entryItems = QEFIEntryStaticList::instance()->entries();

    // Keys are sorted in RBTree
    QList<quint16> keys = m_entryItems.keys();
    m_entries->clear();
    for (int i = 0; i < keys.size(); i++) {
        QEFIEntry &entry = m_entryItems[keys[i]];
        m_entries->addItem(QString::asprintf("[%04X] ", entry.id()) + entry.name());
    }

    m_topLevelLayout->addWidget(m_entries, 3);

    m_buttonLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    m_topLevelLayout->addLayout(m_buttonLayout, 1);

    m_bootTimeoutLabel = new QLabel(QString::asprintf("Timeout: %d second(s)",
        QEFIEntryStaticList::instance()->timeout()), this);
    m_buttonLayout->addWidget(m_bootTimeoutLabel);
    m_buttonLayout->addStretch(1);
    m_rebootTargetButton = new QPushButton(QString::asprintf("Set as reboot target"), this);
    m_buttonLayout->addWidget(m_rebootTargetButton, 0);

    this->setLayout(m_topLevelLayout);
    // TODO: Add a dialog to ask whether the user wants to reboot now
}

QEFIEntryRebootView::~QEFIEntryRebootView()
{
    // TODO: Use smart ptr
    if (m_topLevelLayout != nullptr) {
        delete m_topLevelLayout;
        m_topLevelLayout = nullptr;
    }

    if (m_entries != nullptr) delete m_entries;
    m_entries = nullptr;

    if (m_rebootTargetButton != nullptr) delete m_rebootTargetButton;
    m_rebootTargetButton = nullptr;
    if (m_bootTimeoutLabel != nullptr) delete m_bootTimeoutLabel;
    m_bootTimeoutLabel = nullptr;
}
