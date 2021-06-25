#include "qefientryrebootview.h"

QEFIEntryRebootView::QEFIEntryRebootView(QWidget *parent)
    : QWidget(parent)
{
    // TODO: Add list view and buttons
    m_topLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);

    m_testLabel = new QLabel(QStringLiteral("Test Reboot View"));
    m_topLevelLayout->addWidget(m_testLabel, 1);

    this->setLayout(m_topLevelLayout);
}

QEFIEntryRebootView::~QEFIEntryRebootView()
{
    // TODO: Use smart ptr
    if (m_topLevelLayout != nullptr) {
        delete m_topLevelLayout;
        m_topLevelLayout = nullptr;
    }

    if (m_testLabel != nullptr) delete m_testLabel;
    m_testLabel = nullptr;
}
