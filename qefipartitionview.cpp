#include "qefipartitionview.h"
#include <QHeaderView>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QInputDialog>
#include <QFileDialog>

QEFIPartitionView::QEFIPartitionView(QWidget *parent)
    : QWidget(parent)
    , m_partitionManager(new QEFIPartitionManager(this))
    , m_selectedRow(-1)
{
    setupUI();

    connect(m_partitionManager, &QEFIPartitionManager::partitionsChanged,
            this, &QEFIPartitionView::updatePartitionTable);
    connect(m_partitionManager, &QEFIPartitionManager::mountStatusChanged,
            this, [this](const QString &, bool) { refreshPartitions(); });

    refreshPartitions();
}

QEFIPartitionView::~QEFIPartitionView()
{
}

void QEFIPartitionView::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);

    // Title
    m_titleLabel = new QLabel(tr("EFI Partition Manager"), this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_mainLayout->addWidget(m_titleLabel);

    // Partition table
    m_partitionTable = new QTableWidget(this);
    m_partitionTable->setColumnCount(5);
    m_partitionTable->setHorizontalHeaderLabels(
        QStringList() << tr("Device") << tr("Label") << tr("Size")
                      << tr("File System") << tr("Status"));

    m_partitionTable->horizontalHeader()->setStretchLastSection(false);
    m_partitionTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_partitionTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_partitionTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_partitionTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_partitionTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    m_partitionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_partitionTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_partitionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(m_partitionTable, &QTableWidget::itemSelectionChanged,
            this, &QEFIPartitionView::selectionChanged);

    m_mainLayout->addWidget(m_partitionTable);

    // Buttons
    m_buttonLayout = new QHBoxLayout();

    m_refreshButton = new QPushButton(tr("Refresh"), this);
    connect(m_refreshButton, &QPushButton::clicked,
            this, &QEFIPartitionView::refreshPartitions);
    m_buttonLayout->addWidget(m_refreshButton);

    m_mountButton = new QPushButton(tr("Mount"), this);
    connect(m_mountButton, &QPushButton::clicked,
            this, &QEFIPartitionView::mountSelectedPartition);
    m_buttonLayout->addWidget(m_mountButton);

    m_unmountButton = new QPushButton(tr("Unmount"), this);
    connect(m_unmountButton, &QPushButton::clicked,
            this, &QEFIPartitionView::unmountSelectedPartition);
    m_buttonLayout->addWidget(m_unmountButton);

    m_openButton = new QPushButton(tr("Open"), this);
    connect(m_openButton, &QPushButton::clicked,
            this, &QEFIPartitionView::openMountPoint);
    m_buttonLayout->addWidget(m_openButton);

    m_buttonLayout->addStretch();

    m_mainLayout->addLayout(m_buttonLayout);

    // Status label
    m_statusLabel = new QLabel(this);
    m_mainLayout->addWidget(m_statusLabel);

    updateButtonStates();
}

void QEFIPartitionView::refreshPartitions()
{
    m_statusLabel->setText(tr("Scanning for EFI partitions..."));
    m_partitionManager->refresh();
    updatePartitionTable();
    updateButtonStates();

    QList<QEFIPartitionInfo> efiPartitions = m_partitionManager->getEFIPartitions();
    m_statusLabel->setText(tr("Found %1 EFI partition(s)").arg(efiPartitions.size()));
}

void QEFIPartitionView::updatePartitionTable()
{
    m_partitionTable->setRowCount(0);

    QList<QEFIPartitionInfo> partitions = m_partitionManager->getEFIPartitions();

    for (const auto &partition : partitions) {
        int row = m_partitionTable->rowCount();
        m_partitionTable->insertRow(row);

        m_partitionTable->setItem(row, 0, new QTableWidgetItem(partition.devicePath));
        m_partitionTable->setItem(row, 1, new QTableWidgetItem(partition.label));
        m_partitionTable->setItem(row, 2, new QTableWidgetItem(formatSize(partition.size)));
        m_partitionTable->setItem(row, 3, new QTableWidgetItem(partition.fileSystem));

        QString status = partition.isMounted ? tr("Mounted") : tr("Not Mounted");
        m_partitionTable->setItem(row, 4, new QTableWidgetItem(status));
    }
}

void QEFIPartitionView::mountSelectedPartition()
{
    if (m_selectedRow < 0) {
        QMessageBox::warning(this, tr("No Selection"),
                           tr("Please select a partition to mount."));
        return;
    }

    if (!m_partitionManager->hasPrivileges()) {
        QMessageBox::warning(this, tr("Insufficient Privileges"),
                           tr("Administrator/root privileges are required to mount partitions."));
        return;
    }

    QString devicePath = m_partitionTable->item(m_selectedRow, 0)->text();
    QString mountPoint;
    QString errorMessage;

#ifdef Q_OS_WIN
    // On Windows, let the user choose a drive letter
    QStringList availableLetters;
    for (char letter = 'E'; letter <= 'Z'; ++letter) {
        QString testPath = QString("%1:\\").arg(letter);
        if (!QDir(testPath).exists()) {
            availableLetters << QString(letter);
        }
    }

    if (availableLetters.isEmpty()) {
        QMessageBox::warning(this, tr("No Drive Letters Available"),
                           tr("All drive letters (E-Z) are already in use."));
        return;
    }

    bool ok;
    QString selectedLetter = QInputDialog::getItem(this, tr("Select Drive Letter"),
                                                   tr("Choose a drive letter for the EFI partition:"),
                                                   availableLetters, 0, false, &ok);
    if (!ok || selectedLetter.isEmpty()) {
        return; // User cancelled
    }

    mountPoint = selectedLetter + ":\\";
#else
    // On Unix systems, let the user choose a directory for the mount point
    mountPoint = QFileDialog::getExistingDirectory(
        this,
        tr("Select Mount Point Directory"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (mountPoint.isEmpty()) {
        return; // User cancelled
    }

    // Verify the directory is empty
    QDir mountDir(mountPoint);
    if (!mountDir.isEmpty()) {
        QMessageBox::warning(this, tr("Directory Not Empty"),
                           tr("The selected directory is not empty. Please choose an empty directory for the mount point."));
        return;
    }
#endif

    if (m_partitionManager->mountPartition(devicePath, mountPoint, errorMessage)) {
        QMessageBox::information(this, tr("Success"),
                               tr("Partition mounted at: %1").arg(mountPoint));
    } else {
        QMessageBox::critical(this, tr("Mount Failed"),
                            tr("Failed to mount partition: %1").arg(errorMessage));
        // Refresh to ensure UI state is correct even after error
        refreshPartitions();
    }
}

void QEFIPartitionView::unmountSelectedPartition()
{
    if (m_selectedRow < 0) {
        QMessageBox::warning(this, tr("No Selection"),
                           tr("Please select a partition to unmount."));
        return;
    }

    if (!m_partitionManager->hasPrivileges()) {
        QMessageBox::warning(this, tr("Insufficient Privileges"),
                           tr("Administrator/root privileges are required to unmount partitions."));
        return;
    }

    QString devicePath = m_partitionTable->item(m_selectedRow, 0)->text();
    QString errorMessage;

    if (m_partitionManager->unmountPartition(devicePath, errorMessage)) {
        QMessageBox::information(this, tr("Success"),
                               tr("Partition unmounted successfully."));
    } else {
        QMessageBox::critical(this, tr("Unmount Failed"),
                            tr("Failed to unmount partition: %1").arg(errorMessage));
        // Refresh to ensure UI state is correct even after error
        refreshPartitions();
    }
}

void QEFIPartitionView::openMountPoint()
{
    if (m_selectedRow < 0) {
        QMessageBox::warning(this, tr("No Selection"),
                           tr("Please select a partition."));
        return;
    }

    // Get mount point from partition data
    QString devicePath = m_partitionTable->item(m_selectedRow, 0)->text();
    QList<QEFIPartitionInfo> partitions = m_partitionManager->getEFIPartitions();

    QString mountPoint;
    for (const auto &partition : partitions) {
        if (partition.devicePath == devicePath) {
            mountPoint = partition.mountPoint;
            break;
        }
    }

    if (mountPoint.isEmpty()) {
        QMessageBox::warning(this, tr("Not Mounted"),
                           tr("The selected partition is not mounted."));
        return;
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(mountPoint))) {
        QMessageBox::warning(this, tr("Failed to Open"),
                           tr("Failed to open mount point: %1").arg(mountPoint));
    }
}

void QEFIPartitionView::selectionChanged()
{
    QList<QTableWidgetItem *> selectedItems = m_partitionTable->selectedItems();
    if (!selectedItems.isEmpty()) {
        m_selectedRow = selectedItems.first()->row();

        // Update status label with mount point information if partition is mounted
        QString devicePath = m_partitionTable->item(m_selectedRow, 0)->text();
        QList<QEFIPartitionInfo> partitions = m_partitionManager->getEFIPartitions();

        for (const auto &partition : partitions) {
            if (partition.devicePath == devicePath) {
                if (partition.isMounted && !partition.mountPoint.isEmpty()) {
                    m_statusLabel->setText(tr("Mounted at: %1").arg(partition.mountPoint));
                } else {
                    m_statusLabel->setText(tr("Found %1 EFI partition(s)").arg(partitions.size()));
                }
                break;
            }
        }
    } else {
        m_selectedRow = -1;
        QList<QEFIPartitionInfo> efiPartitions = m_partitionManager->getEFIPartitions();
        m_statusLabel->setText(tr("Found %1 EFI partition(s)").arg(efiPartitions.size()));
    }
    updateButtonStates();
}

void QEFIPartitionView::updateButtonStates()
{
    bool hasSelection = (m_selectedRow >= 0);
    bool isMounted = false;

    if (hasSelection && m_selectedRow < m_partitionTable->rowCount()) {
        QString status = m_partitionTable->item(m_selectedRow, 4)->text();
        isMounted = (status == tr("Mounted"));
    }

    m_mountButton->setEnabled(hasSelection && !isMounted);
    m_unmountButton->setEnabled(hasSelection && isMounted);
    m_openButton->setEnabled(hasSelection && isMounted);
}

QString QEFIPartitionView::formatSize(quint64 bytes)
{
    const quint64 KB = 1024;
    const quint64 MB = 1024 * KB;
    const quint64 GB = 1024 * MB;

    if (bytes >= GB) {
        return QString("%1 GB").arg(bytes / (double)GB, 0, 'f', 2);
    } else if (bytes >= MB) {
        return QString("%1 MB").arg(bytes / (double)MB, 0, 'f', 2);
    } else if (bytes >= KB) {
        return QString("%1 KB").arg(bytes / (double)KB, 0, 'f', 2);
    } else {
        return QString("%1 B").arg(bytes);
    }
}
