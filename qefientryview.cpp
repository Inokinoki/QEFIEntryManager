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
#include <QInputDialog>
#include <QSpinBox>
#include <QDialogButtonBox>

#include "qefientrydetailview.h"
#include "qefiloadoptioneditorview.h"

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
            QString item = QStringLiteral("[%1] %2")
                .arg(entry.id(), 4, 16, QLatin1Char('0'))
                .arg(entry.name());

            if (entry.devicePath().size() > 0) {
                item.append('\n').append(entry.devicePath());
            }

            m_entries->addItem(item);

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

    m_moveUpEntryButton = new QPushButton(tr("Move up"), this);
    m_moveDownEntryButton = new QPushButton(tr("Move down"), this);
    m_setCurrentButton = new QPushButton(tr("Make default"), this);
    m_rebootTargetButton = new QPushButton(tr("Set reboot"), this);
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

    m_bootTimeoutLabel = new QLabel(tr("Timeout: %1 second(s)").arg(
        QEFIEntryStaticList::instance()->timeout()), this);
    m_addButton = new QPushButton(tr("Add"), this);
    m_importButton = new QPushButton(tr("Import"), this);
    m_saveButton = new QPushButton(tr("Save"), this);
    m_resetButton = new QPushButton(tr("Reset"), this);
    m_buttonLayout->addWidget(m_bootTimeoutLabel);
    m_buttonLayout->addWidget(m_addButton);
    m_buttonLayout->addWidget(m_importButton);
    m_buttonLayout->addWidget(m_saveButton);
    m_buttonLayout->addWidget(m_resetButton);
    QObject::connect(m_addButton, &QPushButton::clicked,
                     this, &QEFIEntryView::addClicked);
    QObject::connect(m_importButton, &QPushButton::clicked,
                     this, &QEFIEntryView::importClicked);
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
    if (m_importButton != nullptr) m_importButton->deleteLater();
    if (m_addButton != nullptr) m_addButton->deleteLater();
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
    for (const auto &i: std::as_const(m_order)) {
        if (m_entryItems.contains(i)) {
            QEFIEntry &entry = m_entryItems[i];
            QString item = QStringLiteral("[%1] %2")
                .arg(entry.id(), 4, 16, QLatin1Char('0'))
                .arg(entry.name());

            if (entry.devicePath().size() > 0) {
                item.append('\n').append(entry.devicePath());
            }

            m_entries->addItem(item);
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
        int ret = QMessageBox::warning(this, tr("Reboot to ") +
                                       m_entryItems[m_order[m_selectedItemIndex]].name(),
                                       tr("Do you want to reboot now?"),
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
    QEFIEntryDetailView *m_view;
    QBoxLayout *m_topLevelLayout;
public:
    DetailDialog(QEFIEntry &entry, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        m_view = new QEFIEntryDetailView(entry, this);
        setWindowTitle(entry.name());
        m_topLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);
        m_topLevelLayout->addWidget(m_view);
    }

    ~DetailDialog()
    {
        if (m_topLevelLayout) m_topLevelLayout->deleteLater();
        if (m_view) m_view->deleteLater();
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
            QMessageBox::warning(this, tr("Export failed"),
                tr("Data to export is empty."));
            return;
        }
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
        QString exportFilename = QFileDialog::getSaveFileName(this, tr("Export Boot%1")
	    .arg(m_order[m_selectedItemIndex], 4, 16, QLatin1Char('0')));
        QSaveFile exportFile(exportFilename);
        exportFile.open(QIODevice::WriteOnly);
        exportFile.write(data);
        exportFile.commit();
#else
        QFileDialog::saveFileContent(data,
            QStringLiteral("Boot%1.bin").arg(m_order[m_selectedItemIndex], 4, 16, QLatin1Char('0')));
#endif
    }
}

void QEFIEntryView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    if (m_selectedItemIndex >= 0 || m_selectedItemIndex < m_order.size()) {
        QListWidgetItem *currentItem = m_entries->itemAt(event->pos());
        if (currentItem != nullptr) {
            menu.addSection(QStringLiteral("Boot%1").arg(m_order[m_selectedItemIndex],
							  4, 16, QLatin1Char('0')));

            QEFIEntry &entry = m_entryItems[m_order[m_selectedItemIndex]];
            connect(menu.addAction(entry.isActive() ?
                tr("Disable") : tr("Enable")),
                &QAction::triggered, this, &QEFIEntryView::visibilityClicked);

            connect(menu.addAction(tr("Delete")), &QAction::triggered,
                this, &QEFIEntryView::deleteClicked);

            connect(menu.addAction(tr("Export")), &QAction::triggered,
                this, &QEFIEntryView::exportClicked);

            connect(menu.addAction(tr("Property")), &QAction::triggered,
                this, &QEFIEntryView::detailClicked);

            menu.addSeparator();
        }
    }
    connect(menu.addAction(tr("Add")), &QAction::triggered,
        this, &QEFIEntryView::addClicked);

    connect(menu.addAction(tr("Import")), &QAction::triggered,
        this, &QEFIEntryView::importClicked);

    menu.exec(event->globalPos());
}

void QEFIEntryView::deleteClicked(bool checked)
{
    Q_UNUSED(checked);
    if (m_selectedItemIndex >= 0 || m_selectedItemIndex < m_order.size()) {
        m_order.removeAt(m_selectedItemIndex);
        resetClicked(checked);
    }
}

void QEFIEntryView::importClicked(bool checked)
{
    Q_UNUSED(checked);
    QString filename = QFileDialog::getOpenFileName(this, "Open File");
    if (filename.isNull()) return;

    QFile fileToImport(filename);
    if (!fileToImport.exists()) return;

    fileToImport.open(QIODevice::ReadOnly);
    QByteArray data = fileToImport.readAll();

    // Parse using a QEFILoadOption
    QEFILoadOption loadOption(data);
    if (!loadOption.isValidated()) {
        // Show a warning and stop
        QMessageBox::warning(this, tr("Import failed"),
            tr("Data are invalidated."));
        return;
    }

    // Choose an ID
    QInputDialog dialog(this);
    dialog.setWindowTitle(tr("Choose an ID"));
    dialog.setLabelText(tr("Hex value from 0 to FFFF"));
    dialog.setIntRange(0, 0xFFFF);
    dialog.setIntValue(0x1000);
    dialog.setIntStep(1);
    QSpinBox *spinbox = dialog.findChild<QSpinBox*>();
    if (spinbox) spinbox->setDisplayIntegerBase(16);
    if (dialog.exec() != QDialog::Accepted) {
        // Show a warning and stop
        QMessageBox::warning(this, tr("Import failed"),
            tr("The action is cancelled."));
        return;
    }
    bool validatedBootID = false;
    quint16 bootID = spinbox->text().toInt(&validatedBootID, 16);
    if (!validatedBootID) {
        // Show a warning and stop
        QMessageBox::warning(this, tr("Import failed"),
            tr("The chosen ID is invalidated."));
        return;
    }
    bool orderFound = m_order.contains(bootID);
    if (orderFound) {
        // Override: Show a confirmation
        if (QMessageBox::question(this, tr("Import"),
            tr("Do you want to override Boot%1?").arg(bootID, 4, 16, QLatin1Char('0'))) ==
            QMessageBox::No) {
            // Show a warning and stop
            QMessageBox::warning(this, tr("Import failed"),
                tr("The action is cancelled."));
            return;
        }
    }

    if (!QEFIEntryStaticList::instance()->updateBootEntry(bootID, data)) {
        // Show a warning and stop
        QMessageBox::warning(this, tr("Import failed"),
            tr("Data might be invalidated."));
        return;
    }

    // Check and update the list
    if (!orderFound) m_order.append(bootID);
    m_entryItems = QEFIEntryStaticList::instance()->entries();
    m_selectedItemIndex = -1;
    resetClicked(false);
}

class BootEntryEditorDialog : public QDialog
{
    QEFILoadOptionEditorView *m_view;
    QBoxLayout *m_topLevelLayout;
    QDialogButtonBox *m_buttonBox;

    quint16 m_bootID;
    QByteArray m_loadOptionData;
public:
    BootEntryEditorDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        m_view = new QEFILoadOptionEditorView(nullptr, this);
        setWindowTitle(tr("Add EFI Boot Entry"));
        m_topLevelLayout = new QBoxLayout(QBoxLayout::Down, this);
        m_topLevelLayout->addWidget(m_view);

        m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                           QDialogButtonBox::Cancel);
        m_topLevelLayout->addWidget(m_buttonBox);

        connect(m_buttonBox, &QDialogButtonBox::accepted,
            this, &BootEntryEditorDialog::onAccept);
        connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    quint16 bootID() const { return m_bootID; }
    QByteArray loadOptionData() const { return m_loadOptionData; }

    ~BootEntryEditorDialog()
    {
        if (m_topLevelLayout) m_topLevelLayout->deleteLater();
        if (m_view) m_view->deleteLater();
        if (m_buttonBox) m_buttonBox->deleteLater();
    }

public slots:
    void onAccept() {
        // Init the class from the view
        if (m_view != nullptr) {
            m_bootID = m_view->getBootEntryID();
            // FIXME: This method cannot be called the second time
            m_loadOptionData = m_view->generateLoadOption();
            qDebug() << "Will add Boot Entry " << m_bootID << m_loadOptionData;
        }
        accept();
    }
};

void QEFIEntryView::addClicked(bool checked)
{
    Q_UNUSED(checked);
    BootEntryEditorDialog dialog(this);
    if (dialog.exec() == QDialog::Rejected) return;

    // Get Load Option and add it
    quint16 bootID = dialog.bootID();
    QByteArray data = dialog.loadOptionData();
    if (data.size() == 0) {
        // Error
        QMessageBox::warning(this, tr("Add failed"),
            tr("Data might be invalidated."));
    }

    // Parse using a QEFILoadOption
    QEFILoadOption loadOption(data);
    if (!loadOption.isValidated()) {
        // Show a warning and stop
        QMessageBox::warning(this, tr("Add failed"),
            tr("Data are invalidated."));
        return;
    }

    bool orderFound = m_order.contains(bootID);
    if (orderFound) {
        // Override: Show a confirmation
        if (QMessageBox::question(this, tr("Add"),
            tr("Do you want to override Boot%1?").arg(bootID, 4, 16, QLatin1Char('0'))) ==
            QMessageBox::No) {
            // Show a warning and stop
            QMessageBox::warning(this, tr("Add failed"),
                tr("The action is cancelled."));
            return;
        }
    }

    if (!QEFIEntryStaticList::instance()->updateBootEntry(bootID, data)) {
        // Show a warning and stop
        QMessageBox::warning(this, tr("Add failed"),
            tr("Data might be invalidated."));
        return;
    }

    // Check and update the list
    if (!orderFound) m_order.append(bootID);
    m_entryItems = QEFIEntryStaticList::instance()->entries();
    m_selectedItemIndex = -1;
    resetClicked(false);
}
