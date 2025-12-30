#include "qefipartitionmanager.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTextStream>
#include <QThread>

#if defined(Q_OS_UNIX)
#include <cstring>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

const QString g_efiPartTypeGuid = "c12a7328-f81f-11d2-ba4b-00a0c93ec93b";

// Platform-specific includes for partition information
#ifdef Q_OS_LINUX
#include <linux/fs.h>
#include <linux/hdreg.h>
#ifndef BLKGETSIZE64
#include <linux/ioctl.h>
#define BLKGETSIZE64 _IOR(0x12, 114, size_t)
#endif
#endif

#ifdef Q_OS_FREEBSD
#include <libgeom.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/ucred.h>
#endif

#ifdef Q_OS_DARWIN
#include <IOKit/IOBSD.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOBlockStorageDevice.h>
#include <sys/disk.h>
#include <sys/stat.h>

// macOS disk I/O control definitions
#ifndef DKIOCGETBLOCKCOUNT
#define DKIOCGETBLOCKCOUNT _IOR('d', 25, uint64_t)
#endif
#ifndef DKIOCGETBLOCKSIZE
#define DKIOCGETBLOCKSIZE _IOR('d', 24, uint32_t)
#endif
#endif

#endif

#ifdef Q_OS_WIN
#include <cfgmgr32.h>
#include <devguid.h>
#include <initguid.h>
#include <setupapi.h>
#include <vector>
#include <windows.h>
#include <winioctl.h>

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
#if defined(Q_OS_UNIX)
    return scanPartitionsUnix();
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
#if defined(Q_OS_UNIX)
    return mountPartitionUnix(devicePath, mountPoint, errorMessage);
#elif defined(Q_OS_WIN)
    return mountPartitionWindows(devicePath, mountPoint, errorMessage);
#else
    errorMessage = "Mounting not implemented for this platform";
    return false;
#endif
}

bool QEFIPartitionManager::unmountPartition(const QString &devicePath, QString &errorMessage)
{
#if defined(Q_OS_UNIX)
    return unmountPartitionUnix(devicePath, errorMessage);
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

#if defined(Q_OS_UNIX)
// Helper function to get partition size using ioctl
static quint64 getPartitionSize(const QString &devicePath)
{
    quint64 size = 0;
    int fd = open(devicePath.toUtf8().constData(), O_RDONLY);
    if (fd < 0) {
        return 0;
    }

#ifdef Q_OS_LINUX
    // Use BLKGETSIZE64 ioctl on Linux
    if (ioctl(fd, BLKGETSIZE64, &size) < 0) {
        size = 0;
    }
#elif defined(Q_OS_FREEBSD)
    // Use DIOCGMEDIASIZE on FreeBSD
    off_t mediaSize = 0;
    if (ioctl(fd, DIOCGMEDIASIZE, &mediaSize) >= 0) {
        size = static_cast<quint64>(mediaSize);
    }
#elif defined(Q_OS_DARWIN)
    // On macOS, use DKIOCGETBLOCKCOUNT and DKIOCGETBLOCKSIZE
    uint64_t blockCount = 0;
    uint32_t blockSize = 0;

    if (ioctl(fd, DKIOCGETBLOCKCOUNT, &blockCount) >= 0 && ioctl(fd, DKIOCGETBLOCKSIZE, &blockSize) >= 0) {
        size = blockCount * blockSize;
    }
#endif

    close(fd);
    return size;
}

// Helper function to read a sysfs file (Linux only)
#ifdef Q_OS_LINUX
static QString readSysfsFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    QString content = QString::fromUtf8(file.readAll().trimmed());
    file.close();
    return content;
}

// Helper function to get partition type GUID using blkid (fallback for loop devices)
static QString getPartitionTypeGuidBlkid(const QString &devicePath)
{
    QProcess blkid;
    blkid.start("blkid", QStringList() << "-s" << "PART_ENTRY_TYPE" << "-o" << "value" << devicePath);
    if (!blkid.waitForFinished(5000)) {
        return QString();
    }
    if (blkid.exitCode() != 0) {
        return QString();
    }
    QString output = QString::fromUtf8(blkid.readAllStandardOutput()).trimmed();
    return output;
}

// Helper function to check if partition is EFI by reading partition type GUID from sysfs
// Falls back to blkid for loop devices that may not expose partition_type_guid in sysfs
static bool isEfiPartitionLinux(const QString &deviceName, const QString &devicePath = QString())
{
    QString fullDevicePath = devicePath.isEmpty() ? QString("/dev/%1").arg(deviceName) : devicePath;

    // First try sysfs (preferred method for regular block devices)
    QString sysPath = QString("/sys/class/block/%1/partition").arg(deviceName);
    if (QFile::exists(sysPath)) {
        QString partTypePath = QString("/sys/class/block/%1/partition_type_guid").arg(deviceName);
        QString partType = readSysfsFile(partTypePath);

        if (!partType.isEmpty()) {
            // Remove any hyphens and compare
            partType = partType.toLower().remove('-');
            QString efiGuid = QString(g_efiPartTypeGuid).remove('-');
            return (partType == efiGuid);
        }
    }

    // Fallback to blkid for loop devices or when sysfs doesn't have the GUID
    QString partType = getPartitionTypeGuidBlkid(fullDevicePath);
    if (!partType.isEmpty()) {
        // Remove any hyphens and compare
        partType = partType.toLower().remove('-');
        QString efiGuid = QString(g_efiPartTypeGuid).remove('-');
        return (partType == efiGuid);
    }

    return false;
}

// Helper function to get mount point from /proc/mounts
static QString getMountPoint(const QString &devicePath)
{
    QFile mountsFile("/proc/mounts");
    if (!mountsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream in(&mountsFile);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList fields = line.split(' ');
        if (fields.size() >= 2 && fields[0] == devicePath) {
            mountsFile.close();
            return fields[1];
        }
    }

    mountsFile.close();
    return QString();
}
#endif

#ifdef Q_OS_LINUX
const QList<QEFIPartitionInfo> scanPartitionsLinux()
{
    QList<QEFIPartitionInfo> partitions;
    // Scan /sys/class/block for all block devices
    QDir blockDir("/sys/class/block");
    if (!blockDir.exists()) {
        qWarning() << "Failed to access /sys/class/block";
        return partitions;
    }

    QStringList devices = blockDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &deviceName : devices) {
        // Skip ram disks and device mapper (but allow loop devices)
        if (deviceName.startsWith("ram") || deviceName.startsWith("dm-")) {
            continue;
        }

        QString devicePath = "/dev/" + deviceName;

        // Check if this is a partition (has partition number)
        // For regular devices: sdX1, nvme0n1p1, mmcblk0p1, vda1
        // For loop devices: loop0p1
        QRegularExpression partRe("^(sd[a-z]|nvme\\d+n\\d+|mmcblk\\d+|vd[a-z]|loop\\d+)p?\\d+$");
        bool isPartition = partRe.match(deviceName).hasMatch();

        // Also check if it's a partition by looking for /sys/class/block/<device>/partition
        // This catches loop devices that might not match the regex
        if (!isPartition) {
            QString partitionIndicator = QString("/sys/class/block/%1/partition").arg(deviceName);
            isPartition = QFile::exists(partitionIndicator);
        }

        if (!isPartition) {
            continue;
        }

        // Check if this is an EFI partition (uses blkid fallback for loop devices)
        if (!isEfiPartitionLinux(deviceName, devicePath)) {
            continue;
        }

        QEFIPartitionInfo info;
        info.devicePath = devicePath;
        info.isEFI = true;

        // Get partition size
        info.size = getPartitionSize(devicePath);

        // Extract partition number
        QRegularExpression partNumRe("(\\d+)$");
        auto partNumMatch = partNumRe.match(deviceName);
        if (partNumMatch.hasMatch()) {
            info.partitionNumber = partNumMatch.captured(1).toUInt();
        }

        // Read partition UUID (try sysfs first, fallback to blkid for loop devices)
        QString partUuidPath = QString("/sys/class/block/%1/partition").arg(deviceName);
        QString uuid;
        if (QFile::exists(partUuidPath)) {
            QString uuidPath = QString("/sys/class/block/%1/partition_uuid").arg(deviceName);
            uuid = readSysfsFile(uuidPath);
        }
        // Fallback to blkid if sysfs doesn't have it (e.g., loop devices)
        if (uuid.isEmpty()) {
            QProcess blkid;
            blkid.start("blkid", QStringList() << "-s" << "PART_ENTRY_UUID" << "-o" << "value" << devicePath);
            if (blkid.waitForFinished(5000) && blkid.exitCode() == 0) {
                uuid = QString::fromUtf8(blkid.readAllStandardOutput()).trimmed();
            }
        }
        if (!uuid.isEmpty()) {
            info.partitionGuid = QUuid(uuid);
        }

        // Try to get filesystem label
        QString labelPath = QString("/sys/class/block/%1/label").arg(deviceName);
        info.label = readSysfsFile(labelPath);
        if (info.label.isEmpty()) {
            info.label = "EFI System Partition";
        }

        // Detect filesystem type (typically vfat for EFI)
        info.fileSystem = "vfat";

        // Check if mounted and get mount point
        info.mountPoint = getMountPoint(devicePath);
        info.isMounted = !info.mountPoint.isEmpty();

        partitions.append(info);
    }
    return partitions;
}
#elif defined(Q_OS_FREEBSD)
const QList<QEFIPartitionInfo> scanPartitionsFreeBSD()
{
    QList<QEFIPartitionInfo> partitions;
    // On FreeBSD, use geom library to enumerate partitions
    struct gmesh mesh;
    struct gclass *classp;
    struct ggeom *gp;
    struct gprovider *pp;

    if (geom_gettree(&mesh) != 0) {
        qWarning() << "Failed to get GEOM tree";
        return partitions;
    }

    // Find the PART class (partitions)
    LIST_FOREACH(classp, &mesh.lg_class, lg_class)
    {
        if (strcmp(classp->lg_name, "PART") == 0) {
            LIST_FOREACH(gp, &classp->lg_geom, lg_geom)
            {
                LIST_FOREACH(pp, &gp->lg_provider, lg_provider)
                {
                    // Check if this is an EFI partition by type
                    struct gconfig *gc;
                    bool isEfi = false;

                    LIST_FOREACH(gc, &pp->lg_config, lg_config)
                    {
                        if (strcmp(gc->lg_name, "type") == 0) {
                            QString type = QString(gc->lg_val).toLower();
                            // FreeBSD uses "efi" as the type name for EFI partitions
                            if (type.contains("efi") || type == "c12a7328-f81f-11d2-ba4b-00a0c93ec93b") {
                                isEfi = true;
                            }
                        }
                    }

                    if (!isEfi) {
                        continue;
                    }

                    QEFIPartitionInfo info;
                    info.devicePath = QString("/dev/%1").arg(pp->lg_name);
                    info.isEFI = true;
                    info.size = pp->lg_mediasize;
                    info.label = "EFI System Partition";
                    info.fileSystem = "msdosfs";

                    // Extract partition number from name (e.g., ada0p1 -> 1)
                    QRegularExpression partNumRe("p(\\d+)$");
                    auto match = partNumRe.match(QString(pp->lg_name));
                    if (match.hasMatch()) {
                        info.partitionNumber = match.captured(1).toUInt();
                    }

                    // Check if mounted (read /etc/fstab or use getmntinfo)
                    struct statfs *mntbuf;
                    int mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
                    for (int i = 0; i < mntsize; i++) {
                        if (info.devicePath == QString(mntbuf[i].f_mntfromname)) {
                            info.mountPoint = QString(mntbuf[i].f_mntonname);
                            info.isMounted = true;
                            break;
                        }
                    }

                    partitions.append(info);
                }
            }
        }
    }

    geom_deletetree(&mesh);
    return partitions;
}

#elif defined(Q_OS_DARWIN)
const QList<QEFIPartitionInfo> scanPartitionsMacOS()
{
    QList<QEFIPartitionInfo> partitions;
    // On macOS, scan /dev for diskXsY devices
    QDir devDir("/dev");
    QStringList devices = devDir.entryList(QStringList() << "disk*s*", QDir::System);

    for (const QString &deviceName : devices) {
        QString devicePath = "/dev/" + deviceName;

        // Get partition info - on macOS, we'd need to use IOKit
        // For now, just detect by size and basic checks
        quint64 size = getPartitionSize(devicePath);

        // EFI partitions are typically small (200-600 MB)
        if (size > 0) {
            QEFIPartitionInfo info;
            info.devicePath = devicePath;
            info.size = size;
            info.label = "EFI System Partition";
            info.fileSystem = "msdos";

            // Extract partition number
            QRegularExpression partNumRe("s(\\d+)$");
            auto match = partNumRe.match(deviceName);
            if (match.hasMatch()) {
                info.partitionNumber = match.captured(1).toUInt();
            }

            info.isEFI = true;
            partitions.append(info);
        }
    }

    return partitions;
}
#endif

QList<QEFIPartitionInfo> QEFIPartitionManager::scanPartitionsUnix()
{
    m_partitions =
#ifdef Q_OS_LINUX
        scanPartitionsLinux();
#elif defined(Q_OS_FREEBSD)
        scanPartitionsFreeBSD();
#elif defined(Q_OS_DARWIN)
        scanPartitionsMacOS();
#else
        throw std::runtime_error("Unsupported Unix platform");
#endif
    return m_partitions;
}

bool QEFIPartitionManager::mountPartitionUnix(const QString &devicePath, QString &mountPoint, QString &errorMessage)
{
    // Mount point is required for Unix systems (selected by user via file picker)
    if (mountPoint.isEmpty()) {
        errorMessage = "Mount point is required";
        return false;
    }

    // Create mount point directory if it doesn't exist
    QDir dir;
    if (!dir.exists(mountPoint)) {
        if (!dir.mkpath(mountPoint)) {
            errorMessage = QString("Failed to create mount point: %1").arg(mountPoint);
            return false;
        }
    }

    // Check if directory is empty
    QDir mountDir(mountPoint);
    if (!mountDir.isEmpty()) {
        errorMessage = QString("Mount point directory is not empty: %1").arg(mountPoint);
        return false;
    }

    // Use POSIX mount system call
    int result = -1;

#ifdef Q_OS_LINUX
    // On Linux, use the mount syscall
    const char *source = devicePath.toUtf8().constData();
    const char *target = mountPoint.toUtf8().constData();
    const char *filesystemtype = "vfat"; // EFI partitions are typically vfat
    unsigned long mountflags = 0;
    const void *data = nullptr;

    result = mount(source, target, filesystemtype, mountflags, data);

#elif defined(Q_OS_FREEBSD) || defined(Q_OS_DARWIN)
    // On FreeBSD or macOS, use mount syscall
    const char *source = devicePath.toUtf8().constData();
    const char *target = mountPoint.toUtf8().constData();

    // FreeBSD or macOS mount signature: mount(const char *type, const char *dir, int flags, void *data)
    result = mount("msdosfs", target, 0, (void *)source);
#endif

    if (result != 0) {
        int err = errno;
        errorMessage = QString("Mount failed: %1 (errno: %2)").arg(strerror(err)).arg(err);

        // Clean up empty directory if we created it
        if (dir.isEmpty()) {
            dir.rmdir(mountPoint);
        }

        return false;
    }

    qDebug() << "Successfully mounted" << devicePath << "at" << mountPoint;
    emit mountStatusChanged(devicePath, true);
    refresh();
    return true;
}

bool QEFIPartitionManager::unmountPartitionUnix(const QString &devicePath, QString &errorMessage)
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

    // Use POSIX umount system call
    const char *target = currentMountPoint.toUtf8().constData();

#ifdef Q_OS_LINUX
    int result = umount2(target, 0); // Use umount2 on Linux
#elif defined(Q_OS_FREEBSD) || defined(Q_OS_DARWIN)
    int result = unmount(target, 0); // Use unmount on macOS or FreeBSD
#endif

    if (result != 0) {
        int err = errno;
        errorMessage = QString("Unmount failed: %1 (errno: %2)").arg(strerror(err)).arg(err);
        return false;
    }

    qDebug() << "Successfully unmounted" << devicePath << "from" << currentMountPoint;

    // Try to remove the mount point directory if it's empty
    QDir dir(currentMountPoint);
    if (dir.isEmpty()) {
        dir.rmdir(currentMountPoint);
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

    // First, build a map of all volumes to their disk/partition info
    // This is more efficient than enumerating volumes for each partition
    struct VolumeInfo {
        QString volumeGuid;
        QString mountPoint;
        QString fileSystem;
        DWORD diskNumber;
        LONGLONG startingOffset;
    };
    QList<VolumeInfo> volumeList;

    WCHAR volumeName[MAX_PATH];
    HANDLE hVolume = FindFirstVolumeW(volumeName, ARRAYSIZE(volumeName));

    if (hVolume != INVALID_HANDLE_VALUE) {
        do {
            VolumeInfo volInfo;
            volInfo.volumeGuid = QString::fromWCharArray(volumeName);

            // Remove trailing backslash for CreateFile
            size_t len = wcslen(volumeName);
            bool hadTrailingBackslash = false;
            if (len > 0 && volumeName[len - 1] == L'\\') {
                volumeName[len - 1] = L'\0';
                hadTrailingBackslash = true;
            }

            HANDLE hVol = CreateFileW(volumeName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

            if (hVol != INVALID_HANDLE_VALUE) {
                VOLUME_DISK_EXTENTS extents;
                DWORD returned;

                if (DeviceIoControl(hVol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, &extents, sizeof(extents), &returned, NULL)) {
                    if (extents.NumberOfDiskExtents > 0) {
                        volInfo.diskNumber = extents.Extents[0].DiskNumber;
                        volInfo.startingOffset = extents.Extents[0].StartingOffset.QuadPart;

                        // Get drive letter/mount point
                        WCHAR pathNames[MAX_PATH * 4] = {0}; // Larger buffer for multiple paths
                        DWORD pathLen = 0;

                        // Restore trailing backslash for GetVolumePathNamesForVolumeNameW
                        // This API requires the volume GUID path to have a trailing backslash
                        if (hadTrailingBackslash) {
                            volumeName[len - 1] = L'\\';
                        }

                        // Validate volume name format before calling GetVolumePathNamesForVolumeNameW
                        // Expected format: "\\?\Volume{GUID}\" or "\\?\Volume{GUID}"
                        QString volNameStr = QString::fromWCharArray(volumeName);
                        qDebug() << "Volume GUID:" << volNameStr << "for Disk" << volInfo.diskNumber << "Offset" << volInfo.startingOffset;

                        BOOL pathResult = GetVolumePathNamesForVolumeNameW(volumeName, pathNames, ARRAYSIZE(pathNames), &pathLen);

                        if (!pathResult) {
                            // Only check GetLastError() when the function fails
                            DWORD pathError = GetLastError();

                            QString errorMsg;
                            if (pathError == 123) { // ERROR_INVALID_NAME
                                errorMsg = QString("ERROR_INVALID_NAME - Volume name format is invalid: '%1'").arg(volNameStr);
                            } else {
                                errorMsg = QString("Error %1 - Volume: '%2'").arg(pathError).arg(volNameStr);
                            }

                            qDebug() << "GetVolumePathNamesForVolumeNameW FAILED:" << errorMsg << "Disk" << volInfo.diskNumber << "Offset"
                                     << volInfo.startingOffset;
                        }

                        // Check if we got a valid mount point
                        if (pathResult && pathLen > 1 && pathNames[0] != L'\0') {
                            volInfo.mountPoint = QString::fromWCharArray(pathNames);

                            // Get filesystem type
                            WCHAR fsName[MAX_PATH];
                            if (GetVolumeInformationW(pathNames, NULL, 0, NULL, NULL, NULL, fsName, ARRAYSIZE(fsName))) {
                                volInfo.fileSystem = QString::fromWCharArray(fsName);
                            }

                            qDebug() << "Found mounted volume: Disk" << volInfo.diskNumber << "Offset" << volInfo.startingOffset
                                     << "Mount:" << volInfo.mountPoint << "FS:" << volInfo.fileSystem;
                        } else {
                            // Volume has no mount point (either unmounted or call failed)
                            // Still need to track it for matching with partitions
                            volInfo.mountPoint = ""; // Empty mount point
                            volInfo.fileSystem = ""; // Unknown filesystem until mounted

                            if (pathResult) {
                                qDebug() << "Found unmounted volume: Disk" << volInfo.diskNumber << "Offset" << volInfo.startingOffset << "(no mount points)";
                            }
                        }

                        // ALWAYS add to volumeList - we need to track all volumes
                        volumeList.append(volInfo);
                    }
                } else {
                    qDebug() << "Failed to get disk extents for volume" << volInfo.volumeGuid << "(Error:" << GetLastError() << ")";
                }
                CloseHandle(hVol);
            } else {
                qDebug() << "Failed to open volume" << volInfo.volumeGuid << "(Error:" << GetLastError() << ")";
            }
        } while (FindNextVolumeW(hVolume, volumeName, ARRAYSIZE(volumeName)));
        FindVolumeClose(hVolume);
    }

    qDebug() << "Scanned" << volumeList.size() << "volumes recognized on system.";

    // Enumerate all physical drives using SetupAPI
    HDEVINFO diskClassDevices = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_DISK, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (diskClassDevices == INVALID_HANDLE_VALUE) {
        qWarning() << "Failed to enumerate disk devices";
        m_partitions = partitions;
        return partitions;
    }

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    DWORD deviceIndex = 0;
    while (SetupDiEnumDeviceInterfaces(diskClassDevices, NULL, &GUID_DEVINTERFACE_DISK, deviceIndex, &deviceInterfaceData)) {
        deviceIndex++;

        // Get the required buffer size for device interface detail
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetailW(diskClassDevices, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);

        if (requiredSize == 0) {
            continue;
        }

        // Allocate buffer for device interface detail
        std::vector<BYTE> buffer(requiredSize);
        PSP_DEVICE_INTERFACE_DETAIL_DATA_W detailData = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(buffer.data());
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        // Get device interface detail
        if (!SetupDiGetDeviceInterfaceDetailW(diskClassDevices, &deviceInterfaceData, detailData, requiredSize, NULL, NULL)) {
            continue;
        }

        QString diskPath = QString::fromWCharArray(detailData->DevicePath);

        // Open the disk device
        HANDLE hDisk = CreateFileW(detailData->DevicePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

        if (hDisk == INVALID_HANDLE_VALUE) {
            continue; // Can't open this disk
        }

        // Declare bytesReturned for DeviceIoControl calls
        DWORD bytesReturned = 0;

        // Get the disk number using STORAGE_DEVICE_NUMBER
        STORAGE_DEVICE_NUMBER diskNumber;
        if (!DeviceIoControl(hDisk, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &diskNumber, sizeof(diskNumber), &bytesReturned, NULL)) {
            qDebug() << "Failed to get disk number for" << diskPath;
            CloseHandle(hDisk);
            continue;
        }

        DWORD diskNum = diskNumber.DeviceNumber;
        qDebug() << "Scanning disk" << diskNum << "(" << diskPath << ")";

        // Get drive layout information
        BYTE layoutBuffer[65536]; // Large buffer for drive layout
        DRIVE_LAYOUT_INFORMATION_EX *layout = (DRIVE_LAYOUT_INFORMATION_EX *)layoutBuffer;

        BOOL success = DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, layout, sizeof(layoutBuffer), &bytesReturned, NULL);

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
            PARTITION_INFORMATION_EX &partInfo = layout->PartitionEntry[i];

            // Check if this is an EFI System Partition
            if (IsEqualGUID(partInfo.Gpt.PartitionType, PARTITION_SYSTEM_GUID)) {
                QEFIPartitionInfo info;
                info.isEFI = true;
                info.partitionNumber = partInfo.PartitionNumber;
                info.size = partInfo.PartitionLength.QuadPart;
                info.devicePath = QString("Disk %1 Partition %2").arg(diskNum).arg(partInfo.PartitionNumber);

                // Convert partition GUID
                GUID &guid = partInfo.Gpt.PartitionId;
                info.partitionGuid = QUuid(guid.Data1,
                                           guid.Data2,
                                           guid.Data3,
                                           guid.Data4[0],
                                           guid.Data4[1],
                                           guid.Data4[2],
                                           guid.Data4[3],
                                           guid.Data4[4],
                                           guid.Data4[5],
                                           guid.Data4[6],
                                           guid.Data4[7]);

                // Get partition name (label)
                QString partName = QString::fromWCharArray(partInfo.Gpt.Name);
                if (!partName.isEmpty()) {
                    info.label = partName;
                } else {
                    info.label = "EFI System Partition";
                }

                qDebug() << "Found EFI partition: Disk" << diskNum << "Partition" << partInfo.PartitionNumber
                         << "StartingOffset:" << partInfo.StartingOffset.QuadPart << "Size:" << partInfo.PartitionLength.QuadPart;

                // Look up the mount point from our pre-built volume list
                bool matched = false;
                for (const VolumeInfo &vol : volumeList) {
                    if (vol.diskNumber == diskNum && vol.startingOffset == partInfo.StartingOffset.QuadPart) {
                        info.mountPoint = vol.mountPoint;
                        info.fileSystem = vol.fileSystem;

                        // Only mark as mounted if the volume has a mount point
                        info.isMounted = !vol.mountPoint.isEmpty();

                        if (info.isMounted) {
                            qDebug() << "  -> Matched to mounted volume:" << vol.mountPoint << "FS:" << vol.fileSystem;
                        } else {
                            qDebug() << "  -> Matched to unmounted volume (no mount point)";
                        }
                        matched = true;
                        break;
                    }
                }

                if (!matched) {
                    qDebug() << "  -> No matching volume found in volumeList";
                    qDebug() << "  -> Checked" << volumeList.size() << "volumes for Disk" << diskNum << "Offset" << partInfo.StartingOffset.QuadPart;
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

    // Clean up SetupAPI device info set
    SetupDiDestroyDeviceInfoList(diskClassDevices);

    qDebug() << "Found" << partitions.size() << "EFI partition(s) on Windows using SetupAPI";
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

        HANDLE hVol = CreateFileW(volumeName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

        if (hVol != INVALID_HANDLE_VALUE) {
            VOLUME_DISK_EXTENTS extents;
            DWORD returned;

            if (DeviceIoControl(hVol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, &extents, sizeof(extents), &returned, NULL)) {
                if (extents.NumberOfDiskExtents > 0 && extents.Extents[0].DiskNumber == diskNum) {
                    // Open the disk to get partition info
                    QString diskPath = QString("\\\\.\\PhysicalDrive%1").arg(diskNum);
                    HANDLE hDisk = CreateFileW((LPCWSTR)diskPath.utf16(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

                    if (hDisk != INVALID_HANDLE_VALUE) {
                        BYTE buffer[65536];
                        DRIVE_LAYOUT_INFORMATION_EX *layout = (DRIVE_LAYOUT_INFORMATION_EX *)buffer;
                        DWORD bytesReturned;

                        if (DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, layout, sizeof(buffer), &bytesReturned, NULL)) {
                            for (DWORD i = 0; i < layout->PartitionCount; i++) {
                                if (layout->PartitionEntry[i].PartitionNumber == partNum
                                    && extents.Extents[0].StartingOffset.QuadPart == layout->PartitionEntry[i].StartingOffset.QuadPart) {
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

        if (foundVolume)
            break;

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

    BOOL result = SetVolumeMountPointW((LPCWSTR)mountPoint.utf16(), (LPCWSTR)volumeGuid.utf16());

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

    // Wait for Windows to recognize the mount and update its internal state
    // This is critical - Windows needs time to propagate the volume mount point
    QThread::msleep(500);

    // Force a complete refresh to re-scan all partitions
    refresh();

    // Now emit the signal after refresh is complete
    emit mountStatusChanged(devicePath, true);

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

    // Wait for Windows to recognize the unmount and update its internal state
    QThread::msleep(500);

    // Force a complete refresh to re-scan all partitions
    refresh();

    // Now emit the signal after refresh is complete
    emit mountStatusChanged(devicePath, false);

    return true;
}
#endif
