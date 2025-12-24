#include "qefifileselectiondialog.h"
#include "qpartitioniodevice.h"
#include <QMessageBox>
#include <QDebug>
#include <QHeaderView>
#include <QSharedPointer>

QEFIFileSelectionDialog::QEFIFileSelectionDialog(QWidget *parent)
    : QDialog(parent)
    , m_currentPath("/")
{
    setWindowTitle(tr("Select EFI Boot File"));
    setupUI();
    scanPartitions();
}

QEFIFileSelectionDialog::~QEFIFileSelectionDialog()
{
}

void QEFIFileSelectionDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);

    // Partition selection section
    QHBoxLayout *partitionLayout = new QHBoxLayout();
    m_partitionLabel = new QLabel(tr("EFI Partition:"), this);
    m_partitionComboBox = new QComboBox(this);
    m_scanButton = new QPushButton(tr("Scan"), this);

    partitionLayout->addWidget(m_partitionLabel);
    partitionLayout->addWidget(m_partitionComboBox, 1);
    partitionLayout->addWidget(m_scanButton);

    m_mainLayout->addLayout(partitionLayout);

    // File tree widget
    m_fileTreeWidget = new QTreeWidget(this);
    m_fileTreeWidget->setHeaderLabels(QStringList() << tr("Name") << tr("Size") << tr("Type"));
    m_fileTreeWidget->setRootIsDecorated(true);
    m_fileTreeWidget->setSortingEnabled(true);
    m_fileTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_fileTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_mainLayout->addWidget(m_fileTreeWidget);

    // Status label
    m_statusLabel = new QLabel(this);
    m_mainLayout->addWidget(m_statusLabel);

    // Dialog buttons
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    m_mainLayout->addWidget(m_buttonBox);

    setLayout(m_mainLayout);
    resize(600, 400);

    // Connect signals
    connect(m_scanButton, &QPushButton::clicked, this, &QEFIFileSelectionDialog::scanPartitions);
    connect(m_partitionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QEFIFileSelectionDialog::partitionChanged);
    connect(m_fileTreeWidget, &QTreeWidget::itemDoubleClicked,
            this, &QEFIFileSelectionDialog::fileItemDoubleClicked);
    connect(m_fileTreeWidget, &QTreeWidget::itemClicked,
            this, &QEFIFileSelectionDialog::fileItemClicked);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void QEFIFileSelectionDialog::scanPartitions()
{
    m_statusLabel->setText(tr("Scanning for EFI partitions..."));
    m_partitionComboBox->clear();
    m_fileTreeWidget->clear();

    QEFIPartitionScanner scanner;
    m_partitions = scanner.scanForEFIPartitions();

    if (m_partitions.isEmpty()) {
        m_statusLabel->setText(tr("No EFI partitions found. Please make sure you run this application with administrator/root privileges."));
        QMessageBox::warning(this, tr("No EFI Partitions"),
                             tr("No EFI system partitions were found on this system.\n\n"
                                "Make sure:\n"
                                "1. You are running this application with administrator/root privileges\n"
                                "2. Your system uses UEFI boot mode\n"
                                "3. You have a valid EFI system partition"));
        return;
    }

    for (const auto &partition : m_partitions) {
        QString displayText = QString("%1 - %2 (%3 MB)")
                                  .arg(partition.deviceName)
                                  .arg(partition.partitionLabel)
                                  .arg(partition.partitionSize / 1024 / 1024);
        m_partitionComboBox->addItem(displayText);
    }

    m_statusLabel->setText(tr("Found %1 EFI partition(s)").arg(m_partitions.size()));

    if (m_partitions.size() > 0) {
        m_partitionComboBox->setCurrentIndex(0);
        partitionChanged(0);
    }
}

void QEFIFileSelectionDialog::partitionChanged(int index)
{
    if (index < 0 || index >= m_partitions.size()) {
        return;
    }

    m_selectedPartition = m_partitions[index];
    m_fileTreeWidget->clear();
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    // Close previous filesystem if open
    m_fatFilesystem.reset();

    qDebug() << "Opening partition:" << m_selectedPartition.devicePath
             << "offset:" << m_selectedPartition.partitionOffset
             << "size:" << m_selectedPartition.partitionSize;

    // Create partition IO device
    QSharedPointer<QPartitionIODevice> partitionDevice(
        new QPartitionIODevice(m_selectedPartition.devicePath,
                               m_selectedPartition.partitionOffset,
                               m_selectedPartition.partitionSize));

    if (!partitionDevice->open(QIODevice::ReadOnly)) {
        m_statusLabel->setText(tr("Failed to open partition device"));
        QMessageBox::critical(this, tr("Error"),
                              tr("Failed to open the partition device.\n"
                                 "Make sure you are running with administrator/root privileges."));
        return;
    }

    // Try to create FAT32 filesystem (EFI partitions are typically FAT32)
    m_fatFilesystem.reset(new QFAT32FileSystem(partitionDevice));

    m_currentPath = "/";
    loadDirectory(m_currentPath);

    m_statusLabel->setText(tr("Opened FAT32 filesystem successfully"));
}

void QEFIFileSelectionDialog::loadDirectory(const QString &path)
{
    if (!m_fatFilesystem) {
        return;
    }

    m_fileTreeWidget->clear();
    m_currentPath = path;

    QList<QFATFileInfo> files;
    if (path == "/") {
        files = m_fatFilesystem->listRootDirectory();
    } else {
        files = m_fatFilesystem->listDirectory(path);
    }

    // Add parent directory entry if not root
    if (path != "/") {
        QTreeWidgetItem *parentItem = new QTreeWidgetItem();
        parentItem->setText(0, "..");
        parentItem->setText(1, "");
        parentItem->setText(2, tr("Directory"));
        parentItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
        parentItem->setData(0, Qt::UserRole, true);  // isDirectory
        parentItem->setData(0, Qt::UserRole + 1, "");  // parent path
        m_fileTreeWidget->addTopLevelItem(parentItem);
    }

    // Sort: directories first, then files
    QList<QFATFileInfo> directories;
    QList<QFATFileInfo> regularFiles;

    for (const auto &file : files) {
        if (file.isDirectory) {
            directories.append(file);
        } else {
            regularFiles.append(file);
        }
    }

    // Add directories
    for (const auto &dir : directories) {
        m_fileTreeWidget->addTopLevelItem(createFileItem(dir));
    }

    // Add files
    for (const auto &file : regularFiles) {
        m_fileTreeWidget->addTopLevelItem(createFileItem(file));
    }
}

QTreeWidgetItem *QEFIFileSelectionDialog::createFileItem(const QFATFileInfo &fileInfo)
{
    QTreeWidgetItem *item = new QTreeWidgetItem();

    // Use longName if available, otherwise use name
    QString displayName = fileInfo.longName.isEmpty() ? fileInfo.name : fileInfo.longName;
    item->setText(0, displayName);

    if (fileInfo.isDirectory) {
        item->setText(1, "");
        item->setText(2, tr("Directory"));
        item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    } else {
        item->setText(1, QString::number(fileInfo.size));
        item->setText(2, tr("File"));

        // Set icon based on file extension
        if (displayName.endsWith(".efi", Qt::CaseInsensitive)) {
            item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        } else {
            item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
        }
    }

    // Build full path
    QString fullPath = m_currentPath;
    if (!fullPath.endsWith('/')) {
        fullPath += '/';
    }
    fullPath += displayName;

    // Store file info in item data
    item->setData(0, Qt::UserRole, fileInfo.isDirectory);
    item->setData(0, Qt::UserRole + 1, fullPath);
    item->setData(0, Qt::UserRole + 2, qVariantFromValue(fileInfo));

    return item;
}

void QEFIFileSelectionDialog::fileItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    if (!item) {
        return;
    }

    bool isDirectory = item->data(0, Qt::UserRole).toBool();

    if (isDirectory) {
        QString path = item->data(0, Qt::UserRole + 1).toString();

        if (item->text(0) == "..") {
            // Navigate to parent directory
            QString parentPath = m_currentPath;
            if (parentPath.endsWith('/')) {
                parentPath.chop(1);
            }
            int lastSlash = parentPath.lastIndexOf('/');
            if (lastSlash >= 0) {
                parentPath = parentPath.left(lastSlash);
                if (parentPath.isEmpty()) {
                    parentPath = "/";
                }
            }
            loadDirectory(parentPath);
        } else {
            loadDirectory(path);
        }
    } else {
        // File double-clicked - accept the selection
        accept();
    }
}

void QEFIFileSelectionDialog::fileItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    if (!item) {
        return;
    }

    bool isDirectory = item->data(0, Qt::UserRole).toBool();

    if (!isDirectory && item->text(0) != "..") {
        // Enable OK button for file selection
        m_selectedFilePath = item->data(0, Qt::UserRole + 1).toString();
        m_selectedFileInfo = item->data(0, Qt::UserRole + 2).value<QFATFileInfo>();
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

        m_statusLabel->setText(tr("Selected: %1 (%2 bytes)")
                                   .arg(m_selectedFilePath)
                                   .arg(m_selectedFileInfo.size));
    } else {
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}

void QEFIFileSelectionDialog::refreshFileList()
{
    loadDirectory(m_currentPath);
}

void QEFIFileSelectionDialog::navigateToDirectory(const QString &path)
{
    loadDirectory(path);
}

QByteArray QEFIFileSelectionDialog::getDevicePathData() const
{
    // This method should construct the EFI Device Path structure
    // that can be used in the boot entry creation
    // For now, return the file path as a simple representation
    // In a full implementation, this would construct the proper
    // ACPI/PCI/HD/File device path structure

    QByteArray devicePath;
    // TODO: Implement proper EFI device path construction
    // This would involve creating:
    // 1. ACPI Device Path for the disk controller
    // 2. PCI Device Path if applicable
    // 3. Hard Drive Device Path with partition info
    // 4. File Path Device Path with the selected file path

    return devicePath;
}
