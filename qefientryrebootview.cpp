#include "qefientryrebootview.h"

#include <QDebug>
#include <QMessageBox>
#include <QProcess>

QEFIEntryRebootView::QEFIEntryRebootView(QWidget *parent)
    : QWidget(parent)
{
    // TODO: Add list view and buttons
    m_topLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);

    m_entries = new QListWidget(this);
    m_entryItems = QEFIEntryStaticList::instance()->entries();

    // Keys are sorted in RBTree
    m_entryIds = m_entryItems.keys();
    m_entries->clear();
    m_rebootItemIndex = -1;
    for (int i = 0; i < m_entryIds.size(); i++) {
        QEFIEntry &entry = m_entryItems[m_entryIds[i]];
        m_entries->addItem(QString::asprintf("[%04X] ", entry.id()) + entry.name()
            + (entry.devicePath().size() > 0 ? "\n" + entry.devicePath() : ""));
    }
    QObject::connect(m_entries, &QListWidget::currentRowChanged,
                     this, &QEFIEntryRebootView::entryChanged);
    m_topLevelLayout->addWidget(m_entries, 3);

    m_buttonLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    m_topLevelLayout->addLayout(m_buttonLayout, 1);

    m_bootTimeoutLabel = new QLabel(QString::asprintf("Timeout: %d second(s)",
        QEFIEntryStaticList::instance()->timeout()), this);
    m_buttonLayout->addWidget(m_bootTimeoutLabel);
    m_buttonLayout->addStretch(1);
    m_rebootTargetButton = new QPushButton(QString::asprintf("Set reboot target"), this);
    m_rebootTargetButton->setDisabled(true);
    QObject::connect(m_rebootTargetButton, &QPushButton::clicked,
                     this, &QEFIEntryRebootView::rebootClicked);
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

void QEFIEntryRebootView::entryChanged(int currentRow)
{
    qDebug() << "[EFIRebootView] Clicked: " << currentRow;
    m_rebootItemIndex = currentRow;
    if (m_rebootItemIndex >= m_entryIds.size() || m_rebootItemIndex < 0) {
        m_rebootTargetButton->setDisabled(true);
    } else {
        m_rebootTargetButton->setDisabled(false);
    }
}

void QEFIEntryRebootView::rebootClicked(bool checked)
{
    Q_UNUSED(checked);
    if (m_rebootItemIndex >= 0 || m_rebootItemIndex < m_entryIds.size()) {
        qDebug() << "[EFIRebootView] Set " << m_entryIds[m_rebootItemIndex] << " "
                 << m_entryItems[m_entryIds[m_rebootItemIndex]].name() << " as reboot target";
        // Set BootNext
        QEFIEntryStaticList::instance()->setBootNext(m_entryIds[m_rebootItemIndex]);
        int ret = QMessageBox::warning(this, QStringLiteral("Reboot to ") +
                                       m_entryItems[m_entryIds[m_rebootItemIndex]].name(),
                                       QStringLiteral("Do you want to reboot now?"),
                                       QMessageBox::Yes | QMessageBox::No,
                                       QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            // Reboot now
            qDebug() << "[EFIRebootView] Reboot now";
            QProcess process;
#ifdef Q_OS_WIN
            process.startDetached("shutdown", {"/r"});
#else
            process.startDetached("reboot");
#endif
        } else {
            // Do nothing
            qDebug() << "[EFIRebootView] Reboot later";
        }
        return;
    }
}
