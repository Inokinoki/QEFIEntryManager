#include "qefipartitionscanner.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QStorageInfo>
#include <QRegularExpression>

#ifdef Q_OS_WIN
#include <windows.h>
#include <winioctl.h>
#elif defined(Q_OS_FREEBSD)
#include <sys/disk.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#else  // Linux
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <unistd.h>
#endif

// EFI System Partition GUID: C12A7328-F81F-11D2-BA4B-00A0C93EC93B
const quint8 QEFIPartitionScanner::EFI_SYSTEM_PARTITION_GUID[16] = {
    0x28, 0x73, 0x2A, 0xC1, 0x1F, 0xF8, 0xD2, 0x11,
    0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B
};

QEFIPartitionScanner::QEFIPartitionScanner(QObject *parent)
    : QObject(parent)
{
}

QEFIPartitionScanner::~QEFIPartitionScanner()
{
}

bool QEFIPartitionScanner::isEFIPartition(const quint8 *partitionTypeGUID)
{
    return memcmp(partitionTypeGUID, EFI_SYSTEM_PARTITION_GUID, 16) == 0;
}

QString QEFIPartitionScanner::parsePartitionName(const quint16 *utf16Name, int maxLen)
{
    QString name;
    for (int i = 0; i < maxLen && utf16Name[i] != 0; ++i) {
        name += QChar(utf16Name[i]);
    }
    return name;
}

bool QEFIPartitionScanner::readGPTHeader(const QString &devicePath, GPTHeader &header)
{
    QFile device(devicePath);
    if (!device.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open device:" << devicePath;
        return false;
    }

    // GPT header is at LBA 1 (sector size is typically 512 bytes)
    if (!device.seek(512)) {
        qWarning() << "Failed to seek to GPT header";
        return false;
    }

    QByteArray data = device.read(sizeof(GPTHeader));
    if (data.size() != sizeof(GPTHeader)) {
        qWarning() << "Failed to read GPT header";
        return false;
    }

    memcpy(&header, data.constData(), sizeof(GPTHeader));

    // Verify GPT signature
    if (memcmp(header.signature, "EFI PART", 8) != 0) {
        qDebug() << "Not a valid GPT disk:" << devicePath;
        return false;
    }

    return true;
}

QVector<QEFIPartitionScanInfo> QEFIPartitionScanner::readGPTPartitions(const QString &devicePath)
{
    QVector<QEFIPartitionScanInfo> partitions;

    GPTHeader header;
    if (!readGPTHeader(devicePath, header)) {
        return partitions;
    }

    QFile device(devicePath);
    if (!device.open(QIODevice::ReadOnly)) {
        return partitions;
    }

    // Read partition entries
    quint64 partitionArrayOffset = header.partitionEntryLBA * 512;
    if (!device.seek(partitionArrayOffset)) {
        qWarning() << "Failed to seek to partition array";
        return partitions;
    }

    for (quint32 i = 0; i < header.numPartitionEntries; ++i) {
        QByteArray entryData = device.read(header.partitionEntrySize);
        if (entryData.size() != (int)header.partitionEntrySize) {
            break;
        }

        GPTPartitionEntry entry;
        memcpy(&entry, entryData.constData(), sizeof(GPTPartitionEntry));

        // Check if partition is used (all zeros means unused)
        bool isEmpty = true;
        for (int j = 0; j < 16; ++j) {
            if (entry.partitionTypeGUID[j] != 0) {
                isEmpty = false;
                break;
            }
        }

        if (isEmpty) {
            continue;
        }

        QEFIPartitionScanInfo info;
        info.devicePath = devicePath;
        info.partitionOffset = entry.firstLBA * 512;
        info.partitionSize = (entry.lastLBA - entry.firstLBA + 1) * 512;
        info.partitionLabel = parsePartitionName(entry.partitionName, 36);
        info.isEFI = isEFIPartition(entry.partitionTypeGUID);

        partitions.append(info);
    }

    return partitions;
}

QVector<QEFIPartitionScanInfo> QEFIPartitionScanner::scanLinux()
{
    QVector<QEFIPartitionScanInfo> allPartitions;

#ifdef Q_OS_LINUX
    // Scan /sys/class/block for devices
    QDir blockDir("/sys/class/block");
    if (!blockDir.exists()) {
        qWarning() << "/sys/class/block not found";
        return allPartitions;
    }

    QStringList devices = blockDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &device : devices) {
        // Only scan whole disks (sda, nvme0n1, etc.), not partitions
        QRegularExpression partitionRegex("\\d$");
        if (partitionRegex.match(device).hasMatch()) {
            continue;  // Skip if ends with a number (partition)
        }

        QString devicePath = "/dev/" + device;
        qDebug() << "Scanning device:" << devicePath;

        QVector<QEFIPartitionScanInfo> partitions = readGPTPartitions(devicePath);

        // Update device paths to point to actual partition devices
        for (int i = 0; i < partitions.size(); ++i) {
            QString partitionPath;

            if (device.startsWith("nvme") || device.startsWith("mmcblk")) {
                partitionPath = QString("/dev/%1p%2").arg(device).arg(i + 1);
            } else {
                partitionPath = QString("/dev/%1%2").arg(device).arg(i + 1);
            }

            partitions[i].devicePath = partitionPath;
            partitions[i].deviceName = QString("%1 (%2 MB)")
                                           .arg(partitionPath)
                                           .arg(partitions[i].partitionSize / 1024 / 1024);
        }

        allPartitions.append(partitions);
    }
#endif

    return allPartitions;
}

QVector<QEFIPartitionScanInfo> QEFIPartitionScanner::scanFreeBSD()
{
    QVector<QEFIPartitionScanInfo> allPartitions;

#ifdef Q_OS_FREEBSD
    // Scan /dev for disk devices (ada0, da0, nvd0, etc.)
    QDir devDir("/dev");
    if (!devDir.exists()) {
        return allPartitions;
    }

    QStringList nameFilters;
    nameFilters << "ada[0-9]*" << "da[0-9]*" << "nvd[0-9]*";
    QStringList devices = devDir.entryList(nameFilters, QDir::System);

    for (const QString &device : devices) {
        // Skip partition devices
        if (device.contains('p')) {
            continue;
        }

        QString devicePath = "/dev/" + device;
        qDebug() << "Scanning FreeBSD device:" << devicePath;

        QVector<QEFIPartitionScanInfo> partitions = readGPTPartitions(devicePath);

        // Update device paths
        for (int i = 0; i < partitions.size(); ++i) {
            QString partitionPath = QString("/dev/%1p%2").arg(device).arg(i + 1);
            partitions[i].devicePath = partitionPath;
            partitions[i].deviceName = QString("%1 (%2 MB)")
                                           .arg(partitionPath)
                                           .arg(partitions[i].partitionSize / 1024 / 1024);
        }

        allPartitions.append(partitions);
    }
#endif

    return allPartitions;
}

QVector<QEFIPartitionScanInfo> QEFIPartitionScanner::scanWindows()
{
    QVector<QEFIPartitionScanInfo> allPartitions;

#ifdef Q_OS_WIN
    // Scan physical drives
    for (int driveNum = 0; driveNum < 10; ++driveNum) {
        QString devicePath = QString("\\\\.\\PhysicalDrive%1").arg(driveNum);

        HANDLE hDevice = CreateFileW(
            (LPCWSTR)devicePath.utf16(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (hDevice == INVALID_HANDLE_VALUE) {
            continue;
        }

        CloseHandle(hDevice);

        qDebug() << "Scanning Windows device:" << devicePath;
        QVector<QEFIPartitionScanInfo> partitions = readGPTPartitions(devicePath);

        for (int i = 0; i < partitions.size(); ++i) {
            partitions[i].deviceName = QString("Disk %1 Partition %2 (%3 MB)")
                                           .arg(driveNum)
                                           .arg(i + 1)
                                           .arg(partitions[i].partitionSize / 1024 / 1024);
        }

        allPartitions.append(partitions);
    }
#endif

    return allPartitions;
}

QVector<QEFIPartitionScanInfo> QEFIPartitionScanner::scanForEFIPartitions()
{
    QVector<QEFIPartitionScanInfo> allPartitions;

#ifdef Q_OS_WIN
    allPartitions = scanWindows();
#elif defined(Q_OS_FREEBSD)
    allPartitions = scanFreeBSD();
#else
    allPartitions = scanLinux();
#endif

    // Filter to return only EFI partitions
    QVector<QEFIPartitionScanInfo> efiPartitions;
    for (const auto &partition : allPartitions) {
        if (partition.isEFI) {
            efiPartitions.append(partition);
        }
    }

    qDebug() << "Found" << efiPartitions.size() << "EFI partitions";
    return efiPartitions;
}
