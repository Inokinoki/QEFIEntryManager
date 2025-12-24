#ifndef QEFIPARTITIONSCANNER_H
#define QEFIPARTITIONSCANNER_H

#include <QObject>
#include <QString>
#include <QVector>

// GPT Partition Entry
#pragma pack(push, 1)
struct GPTPartitionEntry {
    quint8  partitionTypeGUID[16];
    quint8  uniquePartitionGUID[16];
    quint64 firstLBA;
    quint64 lastLBA;
    quint64 attributes;
    quint16 partitionName[36];  // UTF-16LE
};

struct GPTHeader {
    char    signature[8];        // "EFI PART"
    quint32 revision;
    quint32 headerSize;
    quint32 headerCRC32;
    quint32 reserved;
    quint64 currentLBA;
    quint64 backupLBA;
    quint64 firstUsableLBA;
    quint64 lastUsableLBA;
    quint8  diskGUID[16];
    quint64 partitionEntryLBA;
    quint32 numPartitionEntries;
    quint32 partitionEntrySize;
    quint32 partitionArrayCRC32;
};
#pragma pack(pop)

struct QEFIPartitionInfo {
    QString devicePath;          // e.g., /dev/sda1, \.\PhysicalDrive0
    QString deviceName;          // User-friendly name
    quint64 partitionOffset;     // Offset in bytes
    quint64 partitionSize;       // Size in bytes
    QString partitionLabel;      // Partition name
    bool isEFI;                  // Is this an EFI system partition?

    QEFIPartitionInfo() : partitionOffset(0), partitionSize(0), isEFI(false) {}
};

class QEFIPartitionScanner : public QObject
{
    Q_OBJECT

public:
    static const quint8 EFI_SYSTEM_PARTITION_GUID[16];

    explicit QEFIPartitionScanner(QObject *parent = nullptr);
    ~QEFIPartitionScanner();

    QVector<QEFIPartitionInfo> scanForEFIPartitions();
    static bool isEFIPartition(const quint8 *partitionTypeGUID);
    static QString parsePartitionName(const quint16 *utf16Name, int maxLen);

private:
    QVector<QEFIPartitionInfo> scanLinux();
    QVector<QEFIPartitionInfo> scanFreeBSD();
    QVector<QEFIPartitionInfo> scanWindows();

    bool readGPTHeader(const QString &devicePath, GPTHeader &header);
    QVector<QEFIPartitionInfo> readGPTPartitions(const QString &devicePath);
};

#endif // QEFIPARTITIONSCANNER_H
