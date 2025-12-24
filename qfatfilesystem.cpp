#include "qfatfilesystem.h"
#include <QDebug>

QFATFilesystem::QFATFilesystem(QObject *parent)
    : QObject(parent)
    , m_device(nullptr)
    , m_partitionOffset(0)
    , m_isOpen(false)
    , m_rootDirFirstCluster(0)
    , m_FATStartSector(0)
    , m_dataStartSector(0)
    , m_totalClusters(0)
{
    memset(&m_bootSector, 0, sizeof(FAT32BootSector));
}

QFATFilesystem::~QFATFilesystem()
{
    close();
}

bool QFATFilesystem::open(const QString &devicePath, quint64 partitionOffset)
{
    close();

    m_device = new QFile(devicePath);
    if (!m_device->open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open device:" << devicePath << m_device->errorString();
        delete m_device;
        m_device = nullptr;
        return false;
    }

    m_partitionOffset = partitionOffset;

    if (!readBootSector()) {
        close();
        return false;
    }

    m_isOpen = true;
    return true;
}

void QFATFilesystem::close()
{
    if (m_device) {
        m_device->close();
        delete m_device;
        m_device = nullptr;
    }
    m_FAT.clear();
    m_isOpen = false;
}

bool QFATFilesystem::isOpen() const
{
    return m_isOpen && m_device != nullptr;
}

bool QFATFilesystem::readBootSector()
{
    if (!m_device || !m_device->seek(m_partitionOffset)) {
        qWarning() << "Failed to seek to partition offset";
        return false;
    }

    QByteArray data = m_device->read(sizeof(FAT32BootSector));
    if (data.size() != sizeof(FAT32BootSector)) {
        qWarning() << "Failed to read boot sector";
        return false;
    }

    memcpy(&m_bootSector, data.constData(), sizeof(FAT32BootSector));

    // Validate boot sector
    if (m_bootSector.bytesPerSector == 0 || m_bootSector.sectorsPerCluster == 0) {
        qWarning() << "Invalid boot sector";
        return false;
    }

    // Calculate key values
    m_rootDirFirstCluster = m_bootSector.rootCluster;
    m_FATStartSector = m_bootSector.reservedSectorCount;

    quint32 fatSize = (m_bootSector.FATSize16 != 0) ? m_bootSector.FATSize16 : m_bootSector.FATSize32;
    m_dataStartSector = m_FATStartSector + (m_bootSector.numFATs * fatSize);

    quint32 totalSectors = (m_bootSector.totalSectors16 != 0) ? m_bootSector.totalSectors16 : m_bootSector.totalSectors32;
    quint32 dataSectors = totalSectors - m_dataStartSector;
    m_totalClusters = dataSectors / m_bootSector.sectorsPerCluster;

    // Read FAT table
    quint64 fatOffset = m_partitionOffset + (m_FATStartSector * m_bootSector.bytesPerSector);
    if (!m_device->seek(fatOffset)) {
        qWarning() << "Failed to seek to FAT";
        return false;
    }

    quint32 fatBytes = fatSize * m_bootSector.bytesPerSector;
    QByteArray fatData = m_device->read(fatBytes);
    if (fatData.size() != (int)fatBytes) {
        qWarning() << "Failed to read FAT table";
        return false;
    }

    // Parse FAT entries (FAT32 uses 4 bytes per entry, but only lower 28 bits)
    m_FAT.resize(fatBytes / 4);
    const quint32 *fatPtr = reinterpret_cast<const quint32 *>(fatData.constData());
    for (int i = 0; i < m_FAT.size(); ++i) {
        m_FAT[i] = fatPtr[i] & 0x0FFFFFFF;
    }

    return true;
}

quint32 QFATFilesystem::getNextCluster(quint32 currentCluster)
{
    if (currentCluster >= (quint32)m_FAT.size()) {
        return 0x0FFFFFFF;  // End of chain
    }

    quint32 nextCluster = m_FAT[currentCluster];

    // Check for end of chain or bad cluster
    if (nextCluster >= 0x0FFFFFF8) {
        return 0x0FFFFFFF;  // End of chain
    }

    return nextCluster;
}

quint32 QFATFilesystem::clusterToSector(quint32 cluster)
{
    if (cluster < 2) {
        return 0;  // Invalid cluster
    }
    return m_dataStartSector + ((cluster - 2) * m_bootSector.sectorsPerCluster);
}

QByteArray QFATFilesystem::readCluster(quint32 clusterNumber)
{
    if (!m_device || clusterNumber < 2) {
        return QByteArray();
    }

    quint32 sector = clusterToSector(clusterNumber);
    quint64 offset = m_partitionOffset + (sector * m_bootSector.bytesPerSector);

    if (!m_device->seek(offset)) {
        qWarning() << "Failed to seek to cluster" << clusterNumber;
        return QByteArray();
    }

    quint32 clusterSize = m_bootSector.sectorsPerCluster * m_bootSector.bytesPerSector;
    return m_device->read(clusterSize);
}

QString QFATFilesystem::parseShortName(const char *name)
{
    QString result;

    // Parse base name (8 chars)
    int i;
    for (i = 0; i < 8 && name[i] != ' '; ++i) {
        result += QChar(name[i]);
    }

    // Parse extension (3 chars)
    bool hasExtension = false;
    for (i = 8; i < 11 && name[i] != ' '; ++i) {
        if (!hasExtension) {
            result += '.';
            hasExtension = true;
        }
        result += QChar(name[i]);
    }

    return result;
}

QString QFATFilesystem::parseLongName(const QVector<FAT32DirectoryEntry> &entries, int startIndex)
{
    // Long filename parsing - simplified version
    // In a full implementation, we would parse LFN entries
    return QString();
}

QVector<QFATFileInfo> QFATFilesystem::readDirectoryCluster(quint32 clusterNumber, const QString &parentPath)
{
    QVector<QFATFileInfo> files;
    QByteArray clusterData = readCluster(clusterNumber);

    if (clusterData.isEmpty()) {
        return files;
    }

    const FAT32DirectoryEntry *entries = reinterpret_cast<const FAT32DirectoryEntry *>(clusterData.constData());
    int entryCount = clusterData.size() / sizeof(FAT32DirectoryEntry);

    for (int i = 0; i < entryCount; ++i) {
        const FAT32DirectoryEntry &entry = entries[i];

        // End of directory
        if (entry.name[0] == 0x00) {
            break;
        }

        // Deleted entry
        if ((quint8)entry.name[0] == 0xE5) {
            continue;
        }

        // Long filename entry (skip for now, use short name)
        if ((entry.attr & ATTR_LONG_NAME) == ATTR_LONG_NAME) {
            continue;
        }

        // Volume ID
        if (entry.attr & ATTR_VOLUME_ID) {
            continue;
        }

        QString name = parseShortName(entry.name);

        // Skip . and ..
        if (name == "." || name == "..") {
            continue;
        }

        QFATFileInfo fileInfo;
        fileInfo.name = name;
        fileInfo.path = parentPath + (parentPath.endsWith('/') ? "" : "/") + name;
        fileInfo.size = entry.fileSize;
        fileInfo.firstCluster = ((quint32)entry.firstClusterHigh << 16) | entry.firstClusterLow;
        fileInfo.isDirectory = (entry.attr & ATTR_DIRECTORY) != 0;
        fileInfo.isHidden = (entry.attr & ATTR_HIDDEN) != 0;
        fileInfo.isSystem = (entry.attr & ATTR_SYSTEM) != 0;

        files.append(fileInfo);
    }

    return files;
}

QVector<QFATFileInfo> QFATFilesystem::listDirectory(const QString &path)
{
    QVector<QFATFileInfo> allFiles;

    if (!isOpen()) {
        return allFiles;
    }

    // For simplicity, start with root directory
    // A full implementation would parse the path
    quint32 currentCluster = m_rootDirFirstCluster;

    while (currentCluster < 0x0FFFFFF8 && currentCluster >= 2) {
        QVector<QFATFileInfo> files = readDirectoryCluster(currentCluster, path);
        allFiles.append(files);

        currentCluster = getNextCluster(currentCluster);
    }

    return allFiles;
}

QByteArray QFATFilesystem::readFile(const QFATFileInfo &fileInfo)
{
    QByteArray data;

    if (!isOpen() || fileInfo.isDirectory) {
        return data;
    }

    quint32 currentCluster = fileInfo.firstCluster;
    quint32 remainingSize = fileInfo.size;
    quint32 clusterSize = m_bootSector.sectorsPerCluster * m_bootSector.bytesPerSector;

    while (currentCluster < 0x0FFFFFF8 && currentCluster >= 2 && remainingSize > 0) {
        QByteArray clusterData = readCluster(currentCluster);
        if (clusterData.isEmpty()) {
            break;
        }

        quint32 bytesToRead = qMin(remainingSize, clusterSize);
        data.append(clusterData.left(bytesToRead));
        remainingSize -= bytesToRead;

        currentCluster = getNextCluster(currentCluster);
    }

    return data;
}

QByteArray QFATFilesystem::readFile(const QString &path)
{
    // Simplified: find file in root directory
    QVector<QFATFileInfo> files = listDirectory("/");

    for (const auto &file : files) {
        if (file.path == path || file.name == path) {
            return readFile(file);
        }
    }

    return QByteArray();
}

QString QFATFilesystem::volumeLabel() const
{
    if (!m_isOpen) {
        return QString();
    }

    QString label;
    for (int i = 0; i < 11 && m_bootSector.volumeLabel[i] != ' '; ++i) {
        label += QChar(m_bootSector.volumeLabel[i]);
    }
    return label.trimmed();
}

QString QFATFilesystem::fileSystemType() const
{
    if (!m_isOpen) {
        return QString();
    }

    QString type;
    for (int i = 0; i < 8 && m_bootSector.fileSystemType[i] != ' '; ++i) {
        type += QChar(m_bootSector.fileSystemType[i]);
    }
    return type.trimmed();
}

quint64 QFATFilesystem::totalSize() const
{
    if (!m_isOpen) {
        return 0;
    }

    quint32 totalSectors = (m_bootSector.totalSectors16 != 0) ?
                           m_bootSector.totalSectors16 : m_bootSector.totalSectors32;
    return (quint64)totalSectors * m_bootSector.bytesPerSector;
}

quint64 QFATFilesystem::freeSpace() const
{
    if (!m_isOpen) {
        return 0;
    }

    quint32 freeClusters = 0;
    for (quint32 cluster : m_FAT) {
        if (cluster == 0) {
            freeClusters++;
        }
    }

    quint32 clusterSize = m_bootSector.sectorsPerCluster * m_bootSector.bytesPerSector;
    return (quint64)freeClusters * clusterSize;
}
