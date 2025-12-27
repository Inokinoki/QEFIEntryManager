#include "qefipartitionmanager.h"
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>
#include <QFileInfo>

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#include <unistd.h>
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

    // Use diskpart or WMI to enumerate partitions
    // This is a simplified implementation
    QProcess diskpart;

    // Create a temporary script for diskpart
    QString scriptPath = QDir::temp().filePath("diskpart_script.txt");
    QFile scriptFile(scriptPath);
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        out << "list partition\n";
        scriptFile.close();
    }

    diskpart.start("diskpart", QStringList() << "/s" << scriptPath);

    if (!diskpart.waitForFinished()) {
        qWarning() << "Failed to execute diskpart";
        scriptFile.remove();
        return partitions;
    }

    QString output = QString::fromUtf8(diskpart.readAllStandardOutput());

    // Parse diskpart output
    // This is a basic implementation and may need enhancement

    scriptFile.remove();
    m_partitions = partitions;
    return partitions;
}

bool QEFIPartitionManager::mountPartitionWindows(const QString &devicePath, QString &mountPoint, QString &errorMessage)
{
    // On Windows, use diskpart or mountvol
    // This is a simplified implementation

    if (mountPoint.isEmpty()) {
        // Find available drive letter
        for (char letter = 'E'; letter <= 'Z'; ++letter) {
            QString driveLetter = QString("%1:").arg(letter);
            if (!QDir(driveLetter).exists()) {
                mountPoint = driveLetter;
                break;
            }
        }
    }

    if (mountPoint.isEmpty()) {
        errorMessage = "No available drive letters";
        return false;
    }

    // Create diskpart script to assign drive letter
    QString scriptPath = QDir::temp().filePath("mount_script.txt");
    QFile scriptFile(scriptPath);
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        out << "select partition " << devicePath << "\n";
        out << "assign letter=" << mountPoint.left(1) << "\n";
        scriptFile.close();
    }

    QProcess diskpart;
    diskpart.start("diskpart", QStringList() << "/s" << scriptPath);

    if (!diskpart.waitForFinished()) {
        errorMessage = "Diskpart command timed out";
        scriptFile.remove();
        return false;
    }

    if (diskpart.exitCode() != 0) {
        errorMessage = QString("Mount failed: %1").arg(QString::fromUtf8(diskpart.readAllStandardError()));
        scriptFile.remove();
        return false;
    }

    scriptFile.remove();
    emit mountStatusChanged(devicePath, true);
    refresh();
    return true;
}

bool QEFIPartitionManager::unmountPartitionWindows(const QString &devicePath, QString &errorMessage)
{
    // Use diskpart to remove drive letter
    QString scriptPath = QDir::temp().filePath("unmount_script.txt");
    QFile scriptFile(scriptPath);
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        out << "select partition " << devicePath << "\n";
        out << "remove\n";
        scriptFile.close();
    }

    QProcess diskpart;
    diskpart.start("diskpart", QStringList() << "/s" << scriptPath);

    if (!diskpart.waitForFinished()) {
        errorMessage = "Diskpart command timed out";
        scriptFile.remove();
        return false;
    }

    if (diskpart.exitCode() != 0) {
        errorMessage = QString("Unmount failed: %1").arg(QString::fromUtf8(diskpart.readAllStandardError()));
        scriptFile.remove();
        return false;
    }

    scriptFile.remove();
    emit mountStatusChanged(devicePath, false);
    refresh();
    return true;
}
#endif
