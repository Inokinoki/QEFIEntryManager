#include "qefipartitionview.h"
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputDialog>
#include <QUrl>
#include <cstring>
#include <qefi.h>
#include <qefientrystaticlist.h>

QEFIPartitionView::QEFIPartitionView(QWidget *parent)
    : QWidget(parent)
    , m_partitionManager(new QEFIPartitionManager(this))
    , m_selectedRow(-1)
{
    setupUI();

    connect(m_partitionManager, &QEFIPartitionManager::partitionsChanged, this, &QEFIPartitionView::updatePartitionTable);
    connect(m_partitionManager, &QEFIPartitionManager::mountStatusChanged, this, [this](const QString &, bool) {
        refreshPartitions();
    });

    refreshPartitions();
}

QEFIPartitionView::~QEFIPartitionView()
{
}

void QEFIPartitionView::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);

    // Partition table
    m_partitionTable = new QTableWidget(this);
    m_partitionTable->setColumnCount(5);
    m_partitionTable->setHorizontalHeaderLabels(QStringList() << tr("Device") << tr("Label") << tr("Size") << tr("File System") << tr("Status"));

    m_partitionTable->horizontalHeader()->setStretchLastSection(false);
    m_partitionTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_partitionTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_partitionTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_partitionTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_partitionTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    m_partitionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_partitionTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_partitionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(m_partitionTable, &QTableWidget::itemSelectionChanged, this, &QEFIPartitionView::selectionChanged);

    m_mainLayout->addWidget(m_partitionTable);

    // Buttons
    m_buttonLayout = new QHBoxLayout();

    m_refreshButton = new QPushButton(tr("Refresh"), this);
    connect(m_refreshButton, &QPushButton::clicked, this, &QEFIPartitionView::refreshPartitions);
    m_buttonLayout->addWidget(m_refreshButton);

    m_mountUnmountButton = new QPushButton(tr("Mount"), this);
    connect(m_mountUnmountButton, &QPushButton::clicked, this, &QEFIPartitionView::toggleMountSelectedPartition);
    m_buttonLayout->addWidget(m_mountUnmountButton);

    m_openButton = new QPushButton(tr("Open"), this);
    connect(m_openButton, &QPushButton::clicked, this, &QEFIPartitionView::openMountPoint);
    m_buttonLayout->addWidget(m_openButton);

    m_createBootEntryButton = new QPushButton(tr("Create boot entry from EFI file"), this);
    connect(m_createBootEntryButton, &QPushButton::clicked, this, &QEFIPartitionView::createBootEntryFromFile);
    m_buttonLayout->addWidget(m_createBootEntryButton);

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

        QString status;
        if (partition.isMounted) {
            status = tr("Mounted at: %1").arg(partition.mountPoint);
        } else {
            status = tr("Not Mounted");
        }
        m_partitionTable->setItem(row, 4, new QTableWidgetItem(status));
    }
}

void QEFIPartitionView::toggleMountSelectedPartition()
{
    if (m_selectedRow < 0) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a partition."));
        return;
    }

    if (!m_partitionManager->hasPrivileges()) {
        QMessageBox::warning(this, tr("Insufficient Privileges"), tr("Administrator/root privileges are required to mount/unmount partitions."));
        return;
    }

    QString devicePath = m_partitionTable->item(m_selectedRow, 0)->text();
    QList<QEFIPartitionInfo> partitions = m_partitionManager->getEFIPartitions();

    bool isMounted = false;
    for (const auto &partition : partitions) {
        if (partition.devicePath == devicePath) {
            isMounted = partition.isMounted;
            break;
        }
    }

    if (isMounted) {
        // Unmount
        QString errorMessage;
        if (m_partitionManager->unmountPartition(devicePath, errorMessage)) {
            QMessageBox::information(this, tr("Success"), tr("Partition unmounted successfully."));
            refreshPartitions();
        } else {
            QMessageBox::critical(this, tr("Unmount Failed"), tr("Failed to unmount partition: %1").arg(errorMessage));
            refreshPartitions();
        }
    } else {
        // Mount
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
            QMessageBox::warning(this, tr("No Drive Letters Available"), tr("All drive letters (E-Z) are already in use."));
            return;
        }

        bool ok;
        QString selectedLetter =
            QInputDialog::getItem(this, tr("Select Drive Letter"), tr("Choose a drive letter for the EFI partition:"), availableLetters, 0, false, &ok);
        if (!ok || selectedLetter.isEmpty()) {
            return; // User cancelled
        }

        mountPoint = selectedLetter + ":\\";
#else
        // On Unix systems, let the user choose a directory for the mount point
        mountPoint = QFileDialog::getExistingDirectory(this,
                                                       tr("Select Mount Point Directory"),
                                                       QDir::homePath(),
                                                       QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        if (mountPoint.isEmpty()) {
            return; // User cancelled
        }

        // Verify the directory is empty
        QDir mountDir(mountPoint);
        if (!mountDir.isEmpty()) {
            QMessageBox::warning(this,
                                 tr("Directory Not Empty"),
                                 tr("The selected directory is not empty. Please choose an empty directory for the mount point."));
            return;
        }
#endif

        if (m_partitionManager->mountPartition(devicePath, mountPoint, errorMessage)) {
            QMessageBox::information(this, tr("Success"), tr("Partition mounted at: %1").arg(mountPoint));
            refreshPartitions();
        } else {
            QMessageBox::critical(this, tr("Mount Failed"), tr("Failed to mount partition: %1").arg(errorMessage));
            refreshPartitions();
        }
    }
}

void QEFIPartitionView::createBootEntryFromFile()
{
    if (m_selectedRow < 0) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a partition."));
        return;
    }

    QString devicePath = m_partitionTable->item(m_selectedRow, 0)->text();
    QList<QEFIPartitionInfo> partitions = m_partitionManager->getEFIPartitions();

    QEFIPartitionInfo selectedPartition;
    bool found = false;
    for (const auto &partition : partitions) {
        if (partition.devicePath == devicePath) {
            selectedPartition = partition;
            found = true;
            break;
        }
    }

    if (!found) {
        QMessageBox::warning(this, tr("Error"), tr("Selected partition not found."));
        return;
    }

    if (!selectedPartition.isMounted || selectedPartition.mountPoint.isEmpty()) {
        QMessageBox::warning(this, tr("Partition Not Mounted"), tr("Please mount the partition first before creating a boot entry."));
        return;
    }

    // Let user select an EFI file
    QString selectedFile = QFileDialog::getOpenFileName(this, tr("Select EFI File"), selectedPartition.mountPoint, tr("EFI Files (*.efi);;All Files (*.*)"));

    if (selectedFile.isEmpty()) {
        return; // User cancelled
    }

    // Check if the file is within the mount point
    QFileInfo fileInfo(selectedFile);
    QString mountPoint = selectedPartition.mountPoint;
    if (!selectedFile.startsWith(mountPoint)) {
        QMessageBox::warning(this, tr("Invalid File"), tr("The selected file is not within the mounted partition."));
        return;
    }

    // Get relative path from mount point (EFI partition root)
    QString relativePath = QDir(mountPoint).relativeFilePath(selectedFile);
    // Handle edge cases (empty or current directory)
    if (relativePath.isEmpty() || relativePath == ".") {
        // File is at the root of the partition
        relativePath = '\\' + QFileInfo(selectedFile).fileName();
    } else {
        // Convert to EFI path format (backslashes)
        relativePath.replace('/', '\\');
        // Ensure it starts with backslash
        if (!relativePath.startsWith('\\')) {
            relativePath = '\\' + relativePath;
        }
    }

    // Create device paths
    // 1. HD device path for the volume
    quint8 signature[16] = {0};
    if (!selectedPartition.partitionGuid.isNull()) {
        QByteArray guidBytes = qefi_rfc4122_to_guid(selectedPartition.partitionGuid.toRfc4122());
        if (guidBytes.size() == 16) {
            memcpy(signature, guidBytes.data(), 16);
        }
    }

    // For GPT partitions, we need partition number, start, and size
    // We'll use partition number from info
    // Start LBA: Use 0 as default (GPT partitions are identified by GUID, so start is less critical)
    quint64 startLba = 0;
    // Size: Convert from bytes to sectors (standard sector size is 512 bytes)
    const quint64 sectorSize = 512;
    quint64 partitionSizeInSectors = selectedPartition.size / sectorSize;
    if (partitionSizeInSectors == 0 && selectedPartition.size > 0) {
        partitionSizeInSectors = 1; // At least 1 sector
    }

    QEFIDevicePathMediaHD *hdDp = new QEFIDevicePathMediaHD(selectedPartition.partitionNumber,
                                                            startLba,
                                                            partitionSizeInSectors,
                                                            signature,
                                                            QEFIDevicePathMediaHD::QEFIDevicePathMediaHDFormat::GPT,
                                                            QEFIDevicePathMediaHD::QEFIDevicePathMediaHDSignatureType::GUID);

    // 2. File device path
    QEFIDevicePathMediaFile *fileDp = new QEFIDevicePathMediaFile(relativePath);

    // Create load option
    QByteArray emptyData;
    QEFILoadOption loadOption(emptyData);
    loadOption.setIsVisible(true);

    // Generate a default name from the file
    QString entryName = QFileInfo(selectedFile).baseName();
    if (entryName.isEmpty()) {
        entryName = tr("Boot Entry from %1").arg(selectedPartition.label);
    }
    loadOption.setName(entryName);

    // Add device paths
    loadOption.addDevicePath(hdDp);
    loadOption.addDevicePath(fileDp);

    // Format the load option
    QByteArray loadOptionData = loadOption.format();
    if (loadOptionData.isEmpty() || !loadOption.isValidated()) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to create boot entry data."));
        return;
    }

    // Get boot entry ID from user
    bool ok;
    quint16 bootID = QInputDialog::getInt(this, tr("Boot Entry ID"), tr("Enter boot entry ID (hex):"), 0x0001, 0x0001, 0xFFFF, 1, &ok);
    if (!ok) {
        return; // User cancelled
    }

    // Save the boot entry
    if (!QEFIEntryStaticList::instance()->updateBootEntry(bootID, loadOptionData)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to save boot entry. The entry ID might be invalid."));
        return;
    }

    // Update boot order if needed
    QList<quint16> order = QEFIEntryStaticList::instance()->order();
    if (!order.contains(bootID)) {
        order.append(bootID);
        QEFIEntryStaticList::instance()->setBootOrder(order);
    }

    QMessageBox::information(this, tr("Success"), tr("Boot entry created successfully with ID: Boot%1").arg(bootID, 4, 16, QLatin1Char('0')).toUpper());
}

void QEFIPartitionView::openMountPoint()
{
    if (m_selectedRow < 0) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a partition."));
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
        QMessageBox::warning(this, tr("Not Mounted"), tr("The selected partition is not mounted."));
        return;
    }

#ifdef Q_OS_WIN
    // On Windows, use a file dialog to browse the mount point
    QString selectedFile = QFileDialog::getOpenFileName(this, tr("Browse EFI Partition - %1").arg(mountPoint), mountPoint, tr("All Files (*.*)"));
    // User can browse and select files, or just close the dialog
    // No need to do anything with the selected file
#else
    // On Unix systems, open the mount point in file manager
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(mountPoint))) {
        QMessageBox::warning(this, tr("Failed to Open"), tr("Failed to open mount point: %1").arg(mountPoint));
    }
#endif
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
        // Status now shows "Mounted at: <path>" or "Not Mounted"
        isMounted = status.startsWith(tr("Mounted at:"));
    }

    m_mountUnmountButton->setEnabled(hasSelection);
    if (hasSelection) {
        if (isMounted) {
            m_mountUnmountButton->setText(tr("Unmount"));
        } else {
            m_mountUnmountButton->setText(tr("Mount"));
        }
    }
    m_openButton->setEnabled(hasSelection && isMounted);
    m_createBootEntryButton->setEnabled(hasSelection && isMounted);
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
