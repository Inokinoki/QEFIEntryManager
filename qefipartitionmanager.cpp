#include "qefipartitionmanager.h"
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>
#include <QFileInfo>
#include <QThread>

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#include <unistd.h>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include <winioctl.h>
#include <initguid.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#include <vector>

// EFI System Partition GUID
DEFINE_GUID(PARTITION_SYSTEM_GUID, 0xC12A7328, 0xF81F, 0x11D2, 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B);
#endif

QEFIPartitionManager::QEFIPartitionManager(QObject *parent)
    : QObject(parent)
{
}

QEFIPartitionManager::~QEFIPartitionManager()
{
}

QList<QEFIPartitionInfo> QEFIPartitionManager::scanPartitions()
{
#ifdef Q_OS_LINUX
    // TODO: Merge Linux and FreeBSD implementations if possible, Unix-like should be similar
    return scanPartitionsLinux();
#elif defined(Q_OS_FREEBSD)
    return scanPartitionsFreeBSD();
#elif defined(Q_OS_WIN)
    return scanPartitionsWindows();
#else
    qWarning() << "Partition scanning not implemented for this platform";
    return QList<QEFIPartitionInfo>();
#endif
}

QList<QEFIPartitionInfo> QEFIPartitionManager::getEFIPartitions()
{
    QList<QEFIPartitionInfo> efiPartitions;
    for (const auto &partition : m_partitions) {
        if (partition.isEFI) {
            efiPartitions.append(partition);
        }
    }
    return efiPartitions;
}

bool QEFIPartitionManager::mountPartition(const QString &devicePath, QString &mountPoint, QString &errorMessage)
{
#ifdef Q_OS_LINUX
    return mountPartitionLinux(devicePath, mountPoint, errorMessage);
#elif defined(Q_OS_FREEBSD)
    return mountPartitionFreeBSD(devicePath, mountPoint, errorMessage);
#elif defined(Q_OS_WIN)
    return mountPartitionWindows(devicePath, mountPoint, errorMessage);
#else
    errorMessage = "Mounting not implemented for this platform";
    return false;
#endif
}

bool QEFIPartitionManager::unmountPartition(const QString &devicePath, QString &errorMessage)
{
#ifdef Q_OS_LINUX
    return unmountPartitionLinux(devicePath, errorMessage);
#elif defined(Q_OS_FREEBSD)
    return unmountPartitionFreeBSD(devicePath, errorMessage);
#elif defined(Q_OS_WIN)
    return unmountPartitionWindows(devicePath, errorMessage);
#else
    errorMessage = "Unmounting not implemented for this platform";
    return false;
#endif
}

bool QEFIPartitionManager::hasPrivileges()
{
#ifdef Q_OS_LINUX
    return geteuid() == 0;
#elif defined(Q_OS_FREEBSD)
    return geteuid() == 0;
#elif defined(Q_OS_WIN)
    // On Windows, check if running with admin privileges
    // This is a simplified check
    return true; // Assume privileges since app requires admin
#else
    return false;
#endif
}

void QEFIPartitionManager::refresh()
{
    m_partitions = scanPartitions();
    emit partitionsChanged();
}

#ifdef Q_OS_LINUX
QList<QEFIPartitionInfo> QEFIPartitionManager::scanPartitionsLinux()
{
    QList<QEFIPartitionInfo> partitions;

    // Use lsblk to get partition information
    QProcess lsblk;
    lsblk.start("lsblk", QStringList()
        << "-o" << "PATH,SIZE,LABEL,PARTTYPE,PARTUUID,FSTYPE,MOUNTPOINT"
        << "-b" << "-P");

    if (!lsblk.waitForFinished()) {
        qWarning() << "Failed to execute lsblk";
        return partitions;
    }

    QString output = QString::fromUtf8(lsblk.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    // EFI System Partition GUID
    const QString efiPartTypeGuid = "c12a7328-f81f-11d2-ba4b-00a0c93ec93b";

    for (const QString &line : lines) {
        QEFIPartitionInfo info;

        // Parse lsblk output
        QRegularExpression pathRe("PATH=\"([^\"]*)\"");
        QRegularExpression sizeRe("SIZE=\"([^\"]*)\"");
        QRegularExpression labelRe("LABEL=\"([^\"]*)\"");
        QRegularExpression parttypeRe("PARTTYPE=\"([^\"]*)\"");
        QRegularExpression partuuidRe("PARTUUID=\"([^\"]*)\"");
        QRegularExpression fstypeRe("FSTYPE=\"([^\"]*)\"");
        QRegularExpression mountpointRe("MOUNTPOINT=\"([^\"]*)\"");

        auto pathMatch = pathRe.match(line);
        if (pathMatch.hasMatch()) {
            info.devicePath = pathMatch.captured(1);
        } else {
            continue;
        }

        auto sizeMatch = sizeRe.match(line);
        if (sizeMatch.hasMatch()) {
            info.size = sizeMatch.captured(1).toULongLong();
        }

        auto labelMatch = labelRe.match(line);
        if (labelMatch.hasMatch()) {
            info.label = labelMatch.captured(1);
        }

        auto parttypeMatch = parttypeRe.match(line);
        if (parttypeMatch.hasMatch()) {
            QString partType = parttypeMatch.captured(1).toLower();
            info.isEFI = (partType == efiPartTypeGuid);
        }

        auto partuuidMatch = partuuidRe.match(line);
        if (partuuidMatch.hasMatch()) {
            info.partitionGuid = QUuid(partuuidMatch.captured(1));
        }

        auto fstypeMatch = fstypeRe.match(line);
        if (fstypeMatch.hasMatch()) {
            info.fileSystem = fstypeMatch.captured(1);
        }

        auto mountpointMatch = mountpointRe.match(line);
        if (mountpointMatch.hasMatch()) {
            info.mountPoint = mountpointMatch.captured(1);
            info.isMounted = !info.mountPoint.isEmpty();
        }

        // Extract partition number from device path
        QRegularExpression partNumRe("\\d+$");
        auto partNumMatch = partNumRe.match(info.devicePath);
        if (partNumMatch.hasMatch()) {
            info.partitionNumber = partNumMatch.captured(0).toUInt();
        }

        // Only add if it's a partition (not a whole disk)
        if (info.devicePath.contains(QRegularExpression("\\d+$"))) {
            partitions.append(info);
        }
    }

    m_partitions = partitions;
    return partitions;
}

bool QEFIPartitionManager::mountPartitionLinux(const QString &devicePath, QString &mountPoint, QString &errorMessage)
{
    // Create mount point in /tmp if not specified
    if (mountPoint.isEmpty()) {
        QString baseName = QFileInfo(devicePath).fileName();
        mountPoint = QString("/tmp/efi_%1").arg(baseName);
    }

    // Create mount point directory if it doesn't exist
    QDir dir;
    if (!dir.exists(mountPoint)) {
        if (!dir.mkpath(mountPoint)) {
            errorMessage = QString("Failed to create mount point: %1").arg(mountPoint);
            return false;
        }
    }

    // Mount the partition
    QProcess mount;
    mount.start("mount", QStringList() << devicePath << mountPoint);

    if (!mount.waitForFinished()) {
        errorMessage = "Mount command timed out";
        return false;
    }

    if (mount.exitCode() != 0) {
        errorMessage = QString("Mount failed: %1").arg(QString::fromUtf8(mount.readAllStandardError()));
        return false;
    }

    emit mountStatusChanged(devicePath, true);
    refresh();
    return true;
}

bool QEFIPartitionManager::unmountPartitionLinux(const QString &devicePath, QString &errorMessage)
{
    QProcess umount;
    umount.start("umount", QStringList() << devicePath);

    if (!umount.waitForFinished()) {
        errorMessage = "Unmount command timed out";
        return false;
    }

    if (umount.exitCode() != 0) {
        errorMessage = QString("Unmount failed: %1").arg(QString::fromUtf8(umount.readAllStandardError()));
        return false;
    }

    emit mountStatusChanged(devicePath, false);
    refresh();
    return true;
}
#endif

#ifdef Q_OS_FREEBSD
QList<QEFIPartitionInfo> QEFIPartitionManager::scanPartitionsFreeBSD()
{
    QList<QEFIPartitionInfo> partitions;

    // Use gpart to list partitions
    QProcess gpart;
    gpart.start("gpart", QStringList() << "status" << "-s");

    if (!gpart.waitForFinished()) {
        qWarning() << "Failed to execute gpart";
        return partitions;
    }

    QString output = QString::fromUtf8(gpart.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    // Parse gpart output
    for (int i = 1; i < lines.size(); ++i) {
        QStringList fields = lines[i].split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (fields.size() < 4) continue;

        QEFIPartitionInfo info;
        info.devicePath = "/dev/" + fields[0];

        // Check if it's an EFI partition
        if (fields.size() > 3 && fields[3].toLower().contains("efi")) {
            info.isEFI = true;
        }

        // Get more details using gpart show
        QProcess gpartShow;
        gpartShow.start("gpart", QStringList() << "show" << "-l" << fields[0]);
        if (gpartShow.waitForFinished()) {
            QString showOutput = QString::fromUtf8(gpartShow.readAllStandardOutput());
            // Parse additional information from gpart show
        }

        partitions.append(info);
    }

    m_partitions = partitions;
    return partitions;
}

bool QEFIPartitionManager::mountPartitionFreeBSD(const QString &devicePath, QString &mountPoint, QString &errorMessage)
{
    // Create mount point in /tmp if not specified
    if (mountPoint.isEmpty()) {
        QString baseName = QFileInfo(devicePath).fileName();
        mountPoint = QString("/tmp/efi_%1").arg(baseName);
    }

    // Create mount point directory
    QDir dir;
    if (!dir.exists(mountPoint)) {
        if (!dir.mkpath(mountPoint)) {
            errorMessage = QString("Failed to create mount point: %1").arg(mountPoint);
            return false;
        }
    }

    // Mount the partition
    QProcess mount;
    mount.start("mount", QStringList() << "-t" << "msdosfs" << devicePath << mountPoint);

    if (!mount.waitForFinished()) {
        errorMessage = "Mount command timed out";
        return false;
    }

    if (mount.exitCode() != 0) {
        errorMessage = QString("Mount failed: %1").arg(QString::fromUtf8(mount.readAllStandardError()));
        return false;
    }

    emit mountStatusChanged(devicePath, true);
    refresh();
    return true;
}

bool QEFIPartitionManager::unmountPartitionFreeBSD(const QString &devicePath, QString &errorMessage)
{
    QProcess umount;
    umount.start("umount", QStringList() << devicePath);

    if (!umount.waitForFinished()) {
        errorMessage = "Unmount command timed out";
        return false;
    }

    if (umount.exitCode() != 0) {
        errorMessage = QString("Unmount failed: %1").arg(QString::fromUtf8(umount.readAllStandardError()));
        return false;
    }

    emit mountStatusChanged(devicePath, false);
    refresh();
    return true;
}
#endif

#ifdef Q_OS_WIN
QList<QEFIPartitionInfo> QEFIPartitionManager::scanPartitionsWindows()
{
    QList<QEFIPartitionInfo> partitions;

    // Enumerate all physical drives
    for (DWORD diskNumber = 0; diskNumber < 32; diskNumber++) {
        QString diskPath = QString("\\\\.\\PhysicalDrive%1").arg(diskNumber);

        HANDLE hDisk = CreateFileW(
            (LPCWSTR)diskPath.utf16(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (hDisk == INVALID_HANDLE_VALUE) {
            continue; // Disk doesn't exist or can't be opened
        }

        // Get drive layout information
        DWORD bytesReturned;
        BYTE buffer[65536]; // Large buffer for drive layout
        DRIVE_LAYOUT_INFORMATION_EX* layout = (DRIVE_LAYOUT_INFORMATION_EX*)buffer;

        BOOL success = DeviceIoControl(
            hDisk,
            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            NULL,
            0,
            layout,
            sizeof(buffer),
            &bytesReturned,
            NULL
        );

        if (!success) {
            CloseHandle(hDisk);
            continue;
        }

        // Check if this is a GPT disk
        if (layout->PartitionStyle != PARTITION_STYLE_GPT) {
            CloseHandle(hDisk);
            continue; // Skip MBR disks
        }

        // Enumerate partitions
        for (DWORD i = 0; i < layout->PartitionCount; i++) {
            PARTITION_INFORMATION_EX& partInfo = layout->PartitionEntry[i];

            // Check if this is an EFI System Partition
            if (IsEqualGUID(partInfo.Gpt.PartitionType, PARTITION_SYSTEM_GUID)) {
                QEFIPartitionInfo info;
                info.isEFI = true;
                info.partitionNumber = partInfo.PartitionNumber;
                info.size = partInfo.PartitionLength.QuadPart;
                info.devicePath = QString("Disk %1 Partition %2").arg(diskNumber).arg(partInfo.PartitionNumber);

                // Convert partition GUID
                GUID& guid = partInfo.Gpt.PartitionId;
                info.partitionGuid = QUuid(
                    guid.Data1, guid.Data2, guid.Data3,
                    guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                    guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]
                );

                // Get partition name (label)
                QString partName = QString::fromWCharArray(partInfo.Gpt.Name);
                if (!partName.isEmpty()) {
                    info.label = partName;
                } else {
                    info.label = "EFI System Partition";
                }

                // Try to find the drive letter
                WCHAR volumeName[MAX_PATH];
                HANDLE hVolume = FindFirstVolumeW(volumeName, ARRAYSIZE(volumeName));

                if (hVolume != INVALID_HANDLE_VALUE) {
                    do {
                        // Remove trailing backslash
                        size_t len = wcslen(volumeName);
                        if (len > 0 && volumeName[len - 1] == L'\\') {
                            volumeName[len - 1] = L'\0';
                        }

                        HANDLE hVol = CreateFileW(
                            volumeName,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL
                        );

                        if (hVol != INVALID_HANDLE_VALUE) {
                            VOLUME_DISK_EXTENTS extents;
                            DWORD returned;

                            if (DeviceIoControl(hVol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                              NULL, 0, &extents, sizeof(extents), &returned, NULL)) {

                                if (extents.NumberOfDiskExtents > 0 &&
                                    extents.Extents[0].DiskNumber == diskNumber) {

                                    // Check if this extent matches our partition
                                    if (extents.Extents[0].StartingOffset.QuadPart == partInfo.StartingOffset.QuadPart) {
                                        // Get drive letter
                                        WCHAR pathNames[MAX_PATH];
                                        DWORD pathLen;

                                        volumeName[len] = L'\\'; // Restore trailing backslash

                                        if (GetVolumePathNamesForVolumeNameW(volumeName, pathNames,
                                                                            ARRAYSIZE(pathNames), &pathLen)) {
                                            QString mountPoint = QString::fromWCharArray(pathNames);
                                            if (!mountPoint.isEmpty() && mountPoint.length() >= 2 && mountPoint[1] == ':') {
                                                info.mountPoint = mountPoint;
                                                info.isMounted = true;

                                                // Get filesystem type
                                                WCHAR fsName[MAX_PATH];
                                                if (GetVolumeInformationW(pathNames, NULL, 0, NULL, NULL, NULL,
                                                                         fsName, ARRAYSIZE(fsName))) {
                                                    info.fileSystem = QString::fromWCharArray(fsName);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            CloseHandle(hVol);
                        }
                    } while (FindNextVolumeW(hVolume, volumeName, ARRAYSIZE(volumeName)));
                    FindVolumeClose(hVolume);
                }

                // Default filesystem if not found
                if (info.fileSystem.isEmpty()) {
                    info.fileSystem = "FAT32";
                }

                partitions.append(info);
            }
        }

        CloseHandle(hDisk);
    }

    qDebug() << "Found" << partitions.size() << "EFI partition(s) on Windows using native API";
    m_partitions = partitions;
    return partitions;
}

bool QEFIPartitionManager::mountPartitionWindows(const QString &devicePath, QString &mountPoint, QString &errorMessage)
{
    // Parse device path to extract disk and partition numbers
    // Expected format: "Disk X Partition Y"
    QRegularExpression diskPartRe("Disk (\\d+) Partition (\\d+)");
    auto match = diskPartRe.match(devicePath);

    if (!match.hasMatch()) {
        errorMessage = "Invalid device path format: " + devicePath;
        return false;
    }

    DWORD diskNum = match.captured(1).toUInt();
    DWORD partNum = match.captured(2).toUInt();

    // First, find the volume GUID for this partition
    QString volumeGuid;
    WCHAR volumeName[MAX_PATH];
    HANDLE hVolume = FindFirstVolumeW(volumeName, ARRAYSIZE(volumeName));

    if (hVolume == INVALID_HANDLE_VALUE) {
        errorMessage = "Failed to enumerate volumes";
        return false;
    }

    bool foundVolume = false;
    do {
        // Remove trailing backslash
        size_t len = wcslen(volumeName);
        if (len > 0 && volumeName[len - 1] == L'\\') {
            volumeName[len - 1] = L'\0';
        }

        HANDLE hVol = CreateFileW(
            volumeName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (hVol != INVALID_HANDLE_VALUE) {
            VOLUME_DISK_EXTENTS extents;
            DWORD returned;

            if (DeviceIoControl(hVol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                              NULL, 0, &extents, sizeof(extents), &returned, NULL)) {

                if (extents.NumberOfDiskExtents > 0 &&
                    extents.Extents[0].DiskNumber == diskNum) {

                    // Open the disk to get partition info
                    QString diskPath = QString("\\\\.\\PhysicalDrive%1").arg(diskNum);
                    HANDLE hDisk = CreateFileW(
                        (LPCWSTR)diskPath.utf16(),
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                    );

                    if (hDisk != INVALID_HANDLE_VALUE) {
                        BYTE buffer[65536];
                        DRIVE_LAYOUT_INFORMATION_EX* layout = (DRIVE_LAYOUT_INFORMATION_EX*)buffer;
                        DWORD bytesReturned;

                        if (DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                          NULL, 0, layout, sizeof(buffer), &bytesReturned, NULL)) {

                            for (DWORD i = 0; i < layout->PartitionCount; i++) {
                                if (layout->PartitionEntry[i].PartitionNumber == partNum &&
                                    extents.Extents[0].StartingOffset.QuadPart ==
                                    layout->PartitionEntry[i].StartingOffset.QuadPart) {

                                    volumeName[len] = L'\\'; // Restore trailing backslash
                                    volumeGuid = QString::fromWCharArray(volumeName);
                                    foundVolume = true;
                                    break;
                                }
                            }
                        }
                        CloseHandle(hDisk);
                    }
                }
            }
            CloseHandle(hVol);
        }

        if (foundVolume) break;

    } while (FindNextVolumeW(hVolume, volumeName, ARRAYSIZE(volumeName)));
    FindVolumeClose(hVolume);

    if (!foundVolume) {
        errorMessage = "Could not find volume for partition";
        return false;
    }

    // Find available drive letter if not specified
    char driveLetter = 0;
    if (mountPoint.isEmpty()) {
        for (char letter = 'E'; letter <= 'Z'; ++letter) {
            QString testPath = QString("%1:\\").arg(letter);
            DWORD attrs = GetFileAttributesW((LPCWSTR)testPath.utf16());
            if (attrs == INVALID_FILE_ATTRIBUTES) {
                // This drive letter is not in use
                driveLetter = letter;
                mountPoint = testPath;
                break;
            }
        }
    } else {
        driveLetter = mountPoint[0].toLatin1();
    }

    if (driveLetter == 0) {
        errorMessage = "No available drive letters (E-Z are all in use)";
        return false;
    }

    // Use SetVolumeMountPoint to properly assign a drive letter
    // Ensure mountPoint has trailing backslash
    mountPoint = QString("%1:\\").arg(QChar(driveLetter));

    // Ensure volumeGuid has trailing backslash
    if (!volumeGuid.endsWith('\\')) {
        volumeGuid += '\\';
    }

    BOOL result = SetVolumeMountPointW(
        (LPCWSTR)mountPoint.utf16(),
        (LPCWSTR)volumeGuid.utf16()
    );

    if (!result) {
        DWORD error = GetLastError();

        // Common error codes:
        // ERROR_ACCESS_DENIED (5) - need admin rights
        // ERROR_ALREADY_EXISTS (183) - mount point already exists
        // ERROR_DIR_NOT_SUPPORTED (336) - directory not supported as mount point

        if (error == 5) {
            errorMessage = "Access denied - administrator privileges required";
        } else if (error == 183) {
            errorMessage = QString("Drive letter %1: is already in use").arg(QChar(driveLetter));
        } else if (error == 336) {
            errorMessage = "Mount point directory is not supported";
        } else {
            errorMessage = QString("Failed to assign drive letter (Windows Error %1)").arg(error);
        }

        qWarning() << "SetVolumeMountPoint failed:" << errorMessage;
        return false;
    }

    qDebug() << "Successfully mounted" << devicePath << "at" << mountPoint;
    emit mountStatusChanged(devicePath, true);

    // Wait a moment for Windows to recognize the mount
    QThread::msleep(500);
    refresh();
    return true;
}

bool QEFIPartitionManager::unmountPartitionWindows(const QString &devicePath, QString &errorMessage)
{
    // Find the current mount point for this partition
    QString currentMountPoint;

    for (const auto &partition : m_partitions) {
        if (partition.devicePath == devicePath && partition.isMounted) {
            currentMountPoint = partition.mountPoint;
            break;
        }
    }

    if (currentMountPoint.isEmpty()) {
        errorMessage = "Partition is not mounted or mount point not found";
        return false;
    }

    // Ensure the mount point has a trailing backslash
    if (!currentMountPoint.endsWith('\\')) {
        currentMountPoint += '\\';
    }

    // Use DeleteVolumeMountPoint to remove the mount
    BOOL result = DeleteVolumeMountPointW((LPCWSTR)currentMountPoint.utf16());

    if (!result) {
        DWORD error = GetLastError();

        if (error == 5) {
            errorMessage = "Access denied - administrator privileges required";
        } else if (error == 3) {
            errorMessage = "The system cannot find the path specified";
        } else {
            errorMessage = QString("Failed to remove drive letter (Windows Error %1)").arg(error);
        }

        qWarning() << "DeleteVolumeMountPoint failed:" << errorMessage;
        return false;
    }

    qDebug() << "Successfully unmounted" << devicePath << "from" << currentMountPoint;
    emit mountStatusChanged(devicePath, false);

    // Wait a moment for Windows to recognize the unmount
    QThread::msleep(500);
    refresh();
    return true;
}
#endif
