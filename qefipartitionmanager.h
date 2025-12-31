#ifndef QEFIPARTITIONMANAGER_H
#define QEFIPARTITIONMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QUuid>

struct QEFIPartitionInfo {
    QString devicePath;        // e.g., /dev/sda1, /dev/nvme0n1p1, or \\.\PHYSICALDRIVE0
    QString mountPoint;        // Current mount point (empty if not mounted)
    QString label;             // Partition label
    QUuid partitionGuid;       // GPT partition GUID
    quint64 size;              // Size in bytes
    quint64 startLba;          // Starting LBA (Logical Block Address) in sectors
    quint32 partitionNumber;   // Partition number
    bool isEFI;                // True if this is an EFI System Partition
    bool isMounted;            // True if currently mounted
    QString fileSystem;        // File system type (e.g., vfat, FAT32)
};

class QEFIPartitionManager : public QObject
{
    Q_OBJECT

public:
    explicit QEFIPartitionManager(QObject *parent = nullptr);
    ~QEFIPartitionManager();

    // Scan for all partitions and identify EFI partitions
    QList<QEFIPartitionInfo> scanPartitions();

    // Get only EFI partitions
    QList<QEFIPartitionInfo> getEFIPartitions();

    // Mount an EFI partition
    // Returns true on success, false on failure
    bool mountPartition(const QString &devicePath, QString &mountPoint, QString &errorMessage);

    // Unmount an EFI partition
    // Returns true on success, false on failure
    bool unmountPartition(const QString &devicePath, QString &errorMessage);

    // Check if we have the necessary privileges to mount/unmount
    bool hasPrivileges();

    // Refresh the partition list
    void refresh();

signals:
    void partitionsChanged();
    void mountStatusChanged(const QString &devicePath, bool mounted);

private:
    QList<QEFIPartitionInfo> m_partitions;

#if defined(Q_OS_UNIX)
    // Unified Unix/POSIX implementation
    QList<QEFIPartitionInfo> scanPartitionsUnix();
    bool mountPartitionUnix(const QString &devicePath, QString &mountPoint, QString &errorMessage);
    bool unmountPartitionUnix(const QString &devicePath, QString &errorMessage);
#endif

#ifdef Q_OS_WIN
    QList<QEFIPartitionInfo> scanPartitionsWindows();
    bool mountPartitionWindows(const QString &devicePath, QString &mountPoint, QString &errorMessage);
    bool unmountPartitionWindows(const QString &devicePath, QString &errorMessage);
#endif
};

#endif // QEFIPARTITIONMANAGER_H
