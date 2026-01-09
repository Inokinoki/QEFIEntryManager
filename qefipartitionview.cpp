#include "qefipartitionview.h"
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QUrl>
#include <QVBoxLayout>
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

    m_createBootEntryButton = new QPushButton(tr("Create"), this);
    m_createBootEntryButton->setToolTip(tr("Create a boot entry from an EFI file"));
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

    QString devicePath = m_partitionTable->item(m_selectedRow, 0)->text();

#ifdef EFI_PARTITION_DISK_IMAGE
    // Check if this is a disk image (test mode) - skip privilege check
    bool isDiskImage = false;
    QList<QEFIPartitionInfo> partitions = m_partitionManager->getEFIPartitions();
    for (const auto &partition : partitions) {
        if (partition.devicePath == devicePath) {
            // Check if device path matches disk image path
            QString diskImagePath = m_partitionManager->getDiskImageFile();
            if (!diskImagePath.isEmpty() && devicePath == diskImagePath) {
                isDiskImage = true;
            }
            break;
        }
    }

    // Only check privileges for real partitions, not disk images
    if (!isDiskImage && !m_partitionManager->hasPrivileges()) {
        QMessageBox::warning(this, tr("Insufficient Privileges"), tr("Administrator/root privileges are required to mount/unmount partitions."));
        return;
    }
#else
    if (!m_partitionManager->hasPrivileges()) {
        QMessageBox::warning(this, tr("Insufficient Privileges"), tr("Administrator/root privileges are required to mount/unmount partitions."));
        return;
    }

    QList<QEFIPartitionInfo> partitions = m_partitionManager->getEFIPartitions();
#endif

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

// Dialog for creating boot entry from EFI file
class EFIBootEntryDialog : public QDialog
{
    Q_OBJECT

    QEFIPartitionInfo m_partition;
    QFormLayout *m_formLayout;
    QSpinBox *m_idSpinBox;
    QLineEdit *m_nameLineEdit;
    QLineEdit *m_optionalDataLineEdit;
    QLineEdit *m_filePathLineEdit;
    QPushButton *m_browseButton;
    QDialogButtonBox *m_buttonBox;
    QString m_selectedFile;

public:
    EFIBootEntryDialog(const QEFIPartitionInfo &partition, QWidget *parent = nullptr)
        : QDialog(parent)
        , m_partition(partition)
    {
        setWindowTitle(tr("Add EFI Boot Entry"));
        setModal(true);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        m_formLayout = new QFormLayout();
        m_formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

        // ID field
        m_idSpinBox = new QSpinBox(this);
        m_idSpinBox->setMinimum(0x0001);
        m_idSpinBox->setMaximum(0xFFFF);
        m_idSpinBox->setValue(0x0001);
        m_idSpinBox->setDisplayIntegerBase(16);
        m_formLayout->addRow(tr("ID:"), m_idSpinBox);

        // Name field
        m_nameLineEdit = new QLineEdit(this);
        m_formLayout->addRow(tr("Name:"), m_nameLineEdit);

        // Optional Data field
        m_optionalDataLineEdit = new QLineEdit(this);
        m_optionalDataLineEdit->setPlaceholderText(tr("Hex format (e.g., 010203)"));
        m_formLayout->addRow(tr("Optional Data:"), m_optionalDataLineEdit);

        // EFI File selection
        QHBoxLayout *fileLayout = new QHBoxLayout();
        m_filePathLineEdit = new QLineEdit(this);
        m_filePathLineEdit->setReadOnly(true);
        m_filePathLineEdit->setPlaceholderText(tr("No file selected"));
        m_browseButton = new QPushButton(tr("Browse..."), this);
        connect(m_browseButton, &QPushButton::clicked, this, &EFIBootEntryDialog::browseEFIFile);
        fileLayout->addWidget(m_filePathLineEdit);
        fileLayout->addWidget(m_browseButton);
        m_formLayout->addRow(tr("EFI File:"), fileLayout);

        mainLayout->addLayout(m_formLayout);

        // Dialog buttons
        m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(m_buttonBox, &QDialogButtonBox::accepted, this, &EFIBootEntryDialog::onAccept);
        connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        mainLayout->addWidget(m_buttonBox);

        // Set default name based on partition
        if (m_nameLineEdit->text().isEmpty()) {
            m_nameLineEdit->setText(tr("Boot Entry from %1").arg(m_partition.label));
        }
    }

    quint16 bootID() const
    {
        return static_cast<quint16>(m_idSpinBox->value());
    }
    QString name() const
    {
        return m_nameLineEdit->text();
    }
    QByteArray optionalData() const
    {
        QString hexText = m_optionalDataLineEdit->text().trimmed();
        if (hexText.isEmpty()) {
            return QByteArray();
        }
        return QByteArray::fromHex(hexText.toLatin1());
    }
    QString selectedFile() const
    {
        return m_selectedFile;
    }

private slots:
    void browseEFIFile()
    {
        QString selectedFile = QFileDialog::getOpenFileName(this, tr("Select EFI File"), m_partition.mountPoint, tr("EFI Files (*.efi);;All Files (*.*)"));
        if (!selectedFile.isEmpty()) {
            // Validate file is within mount point
            QFileInfo fileInfo(selectedFile);
            QDir mountDir(m_partition.mountPoint);
            QString mountPoint = mountDir.absolutePath();
            QString absoluteFilePath = fileInfo.absoluteFilePath();

            QString normalizedMountPoint = QDir::toNativeSeparators(mountPoint);
            QString normalizedFilePath = QDir::toNativeSeparators(absoluteFilePath);

#ifdef Q_OS_WIN
            normalizedMountPoint = normalizedMountPoint.toUpper();
            normalizedFilePath = normalizedFilePath.toUpper();
            if (!normalizedMountPoint.endsWith('\\')) {
                normalizedMountPoint += '\\';
            }
#else
            // On Unix systems, also check canonical paths to handle symlinks
            QString canonicalMountPoint = QDir(mountPoint).canonicalPath();
            QString canonicalFilePath = QDir(absoluteFilePath).canonicalPath();
            qDebug() << "EFI File Validation (canonical paths):";
            qDebug() << "  Canonical mount point:" << canonicalMountPoint;
            qDebug() << "  Canonical file path:" << canonicalFilePath;

            // Use canonical paths for validation if available
            if (!canonicalMountPoint.isEmpty() && !canonicalFilePath.isEmpty() &&
                canonicalFilePath.startsWith(canonicalMountPoint)) {
                normalizedMountPoint = canonicalMountPoint;
                normalizedFilePath = canonicalFilePath;
            }
#endif

            qDebug() << "EFI File Validation:";
            qDebug() << "  Mount point:" << mountPoint;
            qDebug() << "  Normalized mount point:" << normalizedMountPoint;
            qDebug() << "  Selected file:" << selectedFile;
            qDebug() << "  Absolute file path:" << absoluteFilePath;
            qDebug() << "  Normalized file path:" << normalizedFilePath;
            qDebug() << "  Starts with mount point:" << normalizedFilePath.startsWith(normalizedMountPoint);

            if (!normalizedFilePath.startsWith(normalizedMountPoint)) {
                QMessageBox::warning(this, tr("Invalid File"), tr("The selected file is not within the mounted partition."));
                return;
            }

            m_selectedFile = selectedFile;
            m_filePathLineEdit->setText(selectedFile);

            // Update name if empty or default
            if (m_nameLineEdit->text().isEmpty() || m_nameLineEdit->text().startsWith(tr("Boot Entry from"))) {
                QString fileName = QFileInfo(selectedFile).baseName();
                if (!fileName.isEmpty()) {
                    m_nameLineEdit->setText(fileName);
                }
            }
        }
    }

    void onAccept()
    {
        if (m_selectedFile.isEmpty()) {
            QMessageBox::warning(this, tr("No File Selected"), tr("Please select an EFI file."));
            return;
        }

        if (m_nameLineEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Name"), tr("Please enter a name for the boot entry."));
            return;
        }

        accept();
    }
};

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

    // Show dialog
    EFIBootEntryDialog dialog(selectedPartition, this);
    if (dialog.exec() != QDialog::Accepted) {
        return; // User cancelled
    }

    QString selectedFile = dialog.selectedFile();
    if (selectedFile.isEmpty()) {
        return;
    }

    // Get relative path from mount point (EFI partition root)
    QDir mountDir(selectedPartition.mountPoint);
    QString mountPoint = mountDir.absolutePath();
    QString absoluteFilePath = QFileInfo(selectedFile).absoluteFilePath();

    qDebug() << "Boot Entry Creation - Path Calculation:";
    qDebug() << "  Mount point:" << mountPoint;
    qDebug() << "  Selected file:" << selectedFile;
    qDebug() << "  Absolute file path:" << absoluteFilePath;

    QString relativePath = QDir(mountPoint).relativeFilePath(absoluteFilePath);

    qDebug() << "  Relative path (before fixup):" << relativePath;

    // Handle edge cases (empty or current directory)
    if (relativePath.isEmpty() || relativePath == ".") {
        // File is at the root of the partition
        relativePath = '\\' + QFileInfo(absoluteFilePath).fileName();
    } else {
        // Convert to EFI path format (backslashes)
        relativePath.replace('/', '\\');
        // Ensure it starts with backslash
        if (!relativePath.startsWith('\\')) {
            relativePath = '\\' + relativePath;
        }
    }

    qDebug() << "  Relative path (EFI format):" << relativePath;

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
    const quint64 sectorSize = 512;
    quint64 partitionSizeInSectors = selectedPartition.size / sectorSize;
    if (partitionSizeInSectors == 0 && selectedPartition.size > 0) {
        partitionSizeInSectors = 1; // At least 1 sector
    }

    QEFIDevicePathMediaHD *hdDp = new QEFIDevicePathMediaHD(selectedPartition.partitionNumber,
                                                            selectedPartition.startLba,
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
    loadOption.setName(dialog.name());

    QByteArray optionalData = dialog.optionalData();
    if (!optionalData.isEmpty()) {
        loadOption.setOptionalData(optionalData);
    }

    // Add device paths
    loadOption.addDevicePath(hdDp);
    loadOption.addDevicePath(fileDp);

    // Format the load option
    QByteArray loadOptionData = loadOption.format();
    // TODO: Improve LoadOption format and validation
    if (loadOptionData.isEmpty()) {
        qDebug() << "Create boot entry from EFI file failed - format() returned empty data";
        qDebug() << "Load option validated:" << loadOption.isValidated();
        qDebug() << "Load option name:" << loadOption.name();
        qDebug() << "Load option optional data:" << loadOption.optionalData().toHex();
        qDebug() << "Load option device paths:" << loadOption.devicePathList().size();
        qDebug() << "Partition device path:" << hdDp->partitionNumber() << "start:" << hdDp->start() << "size:" << hdDp->size();
        qDebug() << "File device path:" << fileDp->name();
        qDebug() << "Partition GUID:" << selectedPartition.partitionGuid.toString();
        QMessageBox::warning(this, tr("Error"), tr("Failed to create boot entry data - format() returned empty."));
        return;
    }
    qDebug() << "Load option formatted successfully, size:" << loadOptionData.size() << "bytes";

    quint16 bootID = dialog.bootID();
    qDebug() << "About to save boot entry with ID:" << QString("Boot%1").arg(bootID, 4, 16, QLatin1Char('0')).toUpper();
    qDebug() << "Load option data size:" << loadOptionData.size();

    // Check if boot ID already exists
    QList<quint16> order = QEFIEntryStaticList::instance()->order();
    bool orderFound = order.contains(bootID);
    if (orderFound) {
        // Override: Show a confirmation
        if (QMessageBox::question(this, tr("Override Boot Entry"), tr("Do you want to override Boot%1?").arg(bootID, 4, 16, QLatin1Char('0')).toUpper())
            == QMessageBox::No) {
            return;
        }
    }

    // Save the boot entry
    qDebug() << "Calling updateBootEntry for Boot" << QString::number(bootID, 16).rightJustified(4, '0').toUpper();
    bool result = QEFIEntryStaticList::instance()->updateBootEntry(bootID, loadOptionData);
    qDebug() << "updateBootEntry returned:" << result;
    if (!result) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to save boot entry. The entry ID might be invalid."));
        return;
    }

    // Update boot order if needed
    if (!orderFound) {
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

#include "qefipartitionview.moc"
