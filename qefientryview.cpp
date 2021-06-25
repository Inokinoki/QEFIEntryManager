#include "qefientryview.h"

QEFIEntryView::QEFIEntryView(QWidget *parent)
    : QWidget(parent)
{
    // TODO: Add list view and buttons
    m_topLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);

    m_testLabel = new QLabel(QStringLiteral("Test Entry View"));
    m_topLevelLayout->addWidget(m_testLabel, 1);

    this->setLayout(m_topLevelLayout);
}

QEFIEntryView::~QEFIEntryView()
{
    // TODO: Use smart ptr
    if (m_topLevelLayout != nullptr) {
        delete m_topLevelLayout;
        m_topLevelLayout = nullptr;
    }

    if (m_testLabel != nullptr) delete m_testLabel;
    m_testLabel = nullptr;
}

