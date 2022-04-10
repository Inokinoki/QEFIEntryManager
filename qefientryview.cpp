#include "qefientryview.h"

#include "qefientrystaticlist.h"

#include <QDebug>
#include <QMessageBox>
#include <QProcess>

QEFIEntryView::QEFIEntryView(QWidget *parent)
    : QWidget(parent)
{
    // Add list view and buttons
    m_topLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);

    m_entries = new QListWidget(this);
    m_entryItems = QEFIEntryStaticList::instance()->entries();
    m_order = QEFIEntryStaticList::instance()->order();
    m_selectedItemIndex = -1;
    for (int i = 0; i < m_order.size(); i++) {
        if (m_entryItems.contains(m_order[i])) {
            QEFIEntry &entry = m_entryItems[m_order[i]];
            m_entries->addItem(QString::asprintf("[%04X] ", entry.id()) + entry.name()
                + (entry.devicePath().size() > 0 ? "\n" + entry.devicePath() : ""));
            // Get the activation state
            QListWidgetItem *currentItem = m_entries->item(i);
            if (currentItem && !entry.isActive()) {
                currentItem->setForeground(Qt::gray);
            }
        }
    }
    QObject::connect(m_entries, &QListWidget::currentRowChanged,
                     this, &QEFIEntryView::entryChanged);
    m_topLevelLayout->addWidget(m_entries, 3);

    m_buttonLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    m_topLevelLayout->addLayout(m_buttonLayout, 1);

    m_moveUpEntryButton = new QPushButton(QStringLiteral("Move up"), this);
    m_moveDownEntryButton = new QPushButton(QStringLiteral("Move down"), this);
    m_setCurrentButton = new QPushButton(QStringLiteral("Make default"), this);
    m_rebootTargetButton = new QPushButton(QStringLiteral("Set reboot"), this);
    m_rebootTargetButton->setDisabled(true);
    m_buttonLayout->addWidget(m_moveUpEntryButton);
    m_buttonLayout->addWidget(m_moveDownEntryButton);
    m_buttonLayout->addWidget(m_setCurrentButton);
    m_buttonLayout->addWidget(m_rebootTargetButton);
    QObject::connect(m_moveUpEntryButton, &QPushButton::clicked,
                     this, &QEFIEntryView::moveUpClicked);
    QObject::connect(m_moveDownEntryButton, &QPushButton::clicked,
                     this, &QEFIEntryView::moveDownClicked);
    QObject::connect(m_setCurrentButton, &QPushButton::clicked,
                     this, &QEFIEntryView::setCurrentClicked);
    QObject::connect(m_rebootTargetButton, &QPushButton::clicked,
                     this, &QEFIEntryView::rebootClicked);

    m_buttonLayout->addStretch(1);

    m_bootTimeoutLabel = new QLabel(QString::asprintf("Timeout: %d second(s)",
        QEFIEntryStaticList::instance()->timeout()), this);
    m_saveButton = new QPushButton(QStringLiteral("Save"), this);
    m_resetButton = new QPushButton(QStringLiteral("Reset"), this);
    m_buttonLayout->addWidget(m_bootTimeoutLabel);
    m_buttonLayout->addWidget(m_saveButton);
    m_buttonLayout->addWidget(m_resetButton);
    QObject::connect(m_saveButton, &QPushButton::clicked,
                     this, &QEFIEntryView::saveClicked);
    QObject::connect(m_resetButton, &QPushButton::clicked,
                     this, &QEFIEntryView::resetFromStaticListClicked);

    updateButtonState();

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
    if (m_rebootTargetButton != nullptr) delete m_rebootTargetButton;
    if (m_bootTimeoutLabel != nullptr) delete m_bootTimeoutLabel;
}

void QEFIEntryView::entryChanged(int currentRow)
{
    qDebug() << "[EFIEntryView] Clicked: " << currentRow;
    m_selectedItemIndex = currentRow;
    updateButtonState();
}

void QEFIEntryView::resetClicked(bool checked)
{
    m_entries->clear();
    for (int i = 0; i < m_order.size(); i++) {
        if (m_entryItems.contains(m_order[i])) {
            QEFIEntry &entry = m_entryItems[m_order[i]];
            m_entries->addItem(QString::asprintf("[%04X] ", entry.id()) + entry.name()
                + (entry.devicePath().size() > 0 ? "\n" + entry.devicePath() : ""));
        }
    }
    updateButtonState();
}

void QEFIEntryView::saveClicked(bool checked)
{
    qDebug() << "[EFIEntryView] Save button is clicked";
    // TODO: Retrieve from m_order, serialize and save them
    // TODO: Check if there is any changes
    QEFIEntryStaticList::instance()->setBootOrder(m_order);
}

void QEFIEntryView::setCurrentClicked(bool checked)
{
    // Set BootCurrent
    if (m_selectedItemIndex > 0) {
#if QT_VERSION < QT_VERSION_CHECK(5,13,0)
        m_order.swap(m_selectedItemIndex, 0);
#else
        m_order.swapItemsAt(m_selectedItemIndex, 0);
#endif
        resetClicked(checked);
    }
    updateButtonState();
}

void QEFIEntryView::moveUpClicked(bool checked)
{
    // Move the current up
    if (m_selectedItemIndex > 0) {
#if QT_VERSION < QT_VERSION_CHECK(5,13,0)
        m_order.swap(m_selectedItemIndex, m_selectedItemIndex - 1);
#else
        m_order.swapItemsAt(m_selectedItemIndex, m_selectedItemIndex - 1);
#endif
        resetClicked(checked);
    }
    updateButtonState();
}

void QEFIEntryView::moveDownClicked(bool checked)
{
    // Move the current down
    if (m_selectedItemIndex < m_order.size() - 1) {
#if QT_VERSION < QT_VERSION_CHECK(5,13,0)
        m_order.swap(m_selectedItemIndex, m_selectedItemIndex + 1);
#else
        m_order.swapItemsAt(m_selectedItemIndex, m_selectedItemIndex + 1);
#endif
        resetClicked(checked);
    }
    updateButtonState();
}

void QEFIEntryView::updateButtonState()
{
    if (m_selectedItemIndex < m_order.size() && m_selectedItemIndex >= 0) {
        m_moveUpEntryButton->setDisabled(false);
        m_moveDownEntryButton->setDisabled(false);
        m_setCurrentButton->setDisabled(false);
        m_rebootTargetButton->setDisabled(false);
        m_saveButton->setDisabled(false);
        m_resetButton->setDisabled(false);
        if (0 == m_selectedItemIndex) {
            m_moveUpEntryButton->setDisabled(true);
        }
        if (m_order.size() - 1 == m_selectedItemIndex) {
            m_moveDownEntryButton->setDisabled(true);
        }
    } else {
        m_moveUpEntryButton->setDisabled(true);
        m_moveDownEntryButton->setDisabled(true);
        m_setCurrentButton->setDisabled(true);
        m_rebootTargetButton->setDisabled(true);
        m_saveButton->setDisabled(false);
        m_resetButton->setDisabled(false);
    }
}

void QEFIEntryView::resetFromStaticListClicked(bool checked)
{
    m_entryItems = QEFIEntryStaticList::instance()->entries();
    m_order = QEFIEntryStaticList::instance()->order();
    m_selectedItemIndex = -1;
    resetClicked(checked);
}

void QEFIEntryView::rebootClicked(bool checked)
{
    Q_UNUSED(checked);
    if (m_selectedItemIndex >= 0 || m_selectedItemIndex < m_order.size()) {
        qDebug() << "[EFIRebootView] Set " << m_order[m_selectedItemIndex] << " "
                 << m_entryItems[m_order[m_selectedItemIndex]].name() << " as reboot target";
        // Set BootNext
        QEFIEntryStaticList::instance()->setBootNext(m_order[m_selectedItemIndex]);
        int ret = QMessageBox::warning(this, QStringLiteral("Reboot to ") +
                                       m_entryItems[m_order[m_selectedItemIndex]].name(),
                                       QStringLiteral("Do you want to reboot now?"),
                                       QMessageBox::Yes | QMessageBox::No,
                                       QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            // Reboot now
            qDebug() << "[EFIRebootView] Reboot now";
            QProcess process;
#ifdef Q_OS_WIN
            process.startDetached("shutdown", {"/r", "/t", "0"});
#else
            process.startDetached("reboot", {});
#endif
        } else {
            // Do nothing
            qDebug() << "[EFIRebootView] Reboot later";
        }
        return;
    }
}
