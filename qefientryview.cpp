#include "qefientryview.h"

#include "qefientrystaticlist.h"

#include <QDebug>

#include <QMessageBox>
#include <QProcess>

#include <QDialog>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QFileDialog>
#include <QSaveFile>

#include "qefientrydetailview.h"

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
// Fix namsapce change of hex and dec
#define hex Qt::hex
#define dec Qt::dec
#endif

QEFIEntryView::QEFIEntryView(QWidget *parent)
    : QWidget(parent)
{
    // Add list view and buttons
    m_topLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);

    m_entries = new QListWidget(this);
    m_entries->setContextMenuPolicy(Qt::DefaultContextMenu);
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
    if (m_topLevelLayout != nullptr) {
        m_topLevelLayout->deleteLater();
        m_topLevelLayout = nullptr;
    }

    if (m_entries != nullptr) m_entries->deleteLater();
    m_entries = nullptr;

    // Seems that we have no ownership on it
   if (m_buttonLayout != nullptr) m_buttonLayout->deleteLater();

    if (m_moveUpEntryButton != nullptr) m_moveUpEntryButton->deleteLater();
    if (m_moveDownEntryButton != nullptr) m_moveDownEntryButton->deleteLater();
    if (m_setCurrentButton != nullptr) m_setCurrentButton->deleteLater();
    if (m_saveButton != nullptr) m_saveButton->deleteLater();
    if (m_resetButton != nullptr) m_resetButton->deleteLater();
    if (m_rebootTargetButton != nullptr) m_rebootTargetButton->deleteLater();
    if (m_bootTimeoutLabel != nullptr) m_bootTimeoutLabel->deleteLater();
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

void QEFIEntryView::visibilityClicked(bool checked)
{
    Q_UNUSED(checked);
    if (m_selectedItemIndex >= 0 || m_selectedItemIndex < m_order.size()) {
        qDebug() << "[EFIRebootView] Set " << m_order[m_selectedItemIndex] << " "
                 << m_entryItems[m_order[m_selectedItemIndex]].name() << " visibility";
        // Set Attribute
        QEFIEntry &entry = m_entryItems[m_order[m_selectedItemIndex]];
        bool visibility = entry.loadOption()->isVisible();
        if (QEFIEntryStaticList::instance()->setBootVisibility(
            m_order[m_selectedItemIndex], !visibility)) {
            // Already set visibility successful

            // Set the load option visibility
            QEFIEntryStaticList::instance()->entries()[m_order[m_selectedItemIndex]].
                setActive(!visibility);

            // Set the activation state
            QListWidgetItem *currentItem = m_entries->item(m_selectedItemIndex);
            if (currentItem != nullptr) {
                currentItem->setForeground(visibility ? Qt::gray : Qt::black);
            }
        }
    }
}

class DetailDialog : public QDialog
{
    QEFIEntryDetailView m_view;
    QBoxLayout *m_topLevelLayout;
public:
    DetailDialog(QEFIEntry &entry, QWidget *parent = nullptr)
        : QDialog(parent), m_view(entry, this)
    {
        setWindowTitle(entry.name());
        m_topLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);
        m_topLevelLayout->addWidget(&m_view);
    }

    ~DetailDialog()
    {
        if (m_topLevelLayout) m_topLevelLayout->deleteLater();
    }
};

void QEFIEntryView::detailClicked(bool checked)
{
    Q_UNUSED(checked);
    if (m_selectedItemIndex >= 0 || m_selectedItemIndex < m_order.size()) {
        DetailDialog dialog(m_entryItems[m_order[m_selectedItemIndex]], this);
        dialog.exec();
    }
}

void QEFIEntryView::exportClicked(bool checked)
{
    Q_UNUSED(checked);
    if (m_selectedItemIndex >= 0 || m_selectedItemIndex < m_order.size()) {
        QByteArray data = QEFIEntryStaticList::instance()->
            getRawData(m_order[m_selectedItemIndex]);
        if (data.size() == 0) {
            // Show a warning and stop
            QMessageBox::warning(this, QStringLiteral("Export failed"),
                QStringLiteral("Data to export is empty."));
            return;
        }
#if QT_VERSION < QT_VERSION_CHECK(5,14,0)
        QString exportFilename = QFileDialog::getSaveFileName(this,
            QString::asprintf("Export Boot%04X", m_order[m_selectedItemIndex]));
        QSaveFile exportFile(exportFilename);
        exportFile.open(QIODevice::WriteOnly);
        exportFile.write(data);
        exportFile.commit();
#else
        QFileDialog::saveFileContent(data,
            QString::asprintf("Boot%04X.bin", m_order[m_selectedItemIndex]));
#endif
    }
}

void QEFIEntryView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    if (m_selectedItemIndex >= 0 || m_selectedItemIndex < m_order.size()) {
        QListWidgetItem *currentItem = m_entries->itemAt(event->pos());
        if (currentItem != nullptr) {
            menu.addSection(QString::asprintf("Boot%04X", m_order[m_selectedItemIndex]));

            QEFIEntry &entry = m_entryItems[m_order[m_selectedItemIndex]];
            connect(menu.addAction(entry.isActive() ?
                QStringLiteral("Disable") : QStringLiteral("Enable")),
                &QAction::triggered, this, &QEFIEntryView::visibilityClicked);

            // TODO: Allow to delete
            // menu.addAction(QStringLiteral("Delete"));

            connect(menu.addAction(QStringLiteral("Export")), &QAction::triggered,
                this, &QEFIEntryView::exportClicked);

            connect(menu.addAction(QStringLiteral("Property")), &QAction::triggered,
                this, &QEFIEntryView::detailClicked);

            menu.addSeparator();
        }
    }
    // TODO: Allow to add new one
    // menu.addAction(QStringLiteral("Add"));

    // TODO: Allow to import
    // menu.addAction(QStringLiteral("Import"));

    menu.exec(event->globalPos());
}
