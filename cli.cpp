#include "cli.h"
#include "helpers.h"
#include "qefi.h"
#include "qefientry.h"
#include "qefientrystaticlist.h"
#include "qefipartitionmanager.h"
#include "version.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>
#include <QtEndian>

#include <cstring>
#include <iostream>

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#define hex Qt::hex
#define dec Qt::dec
#endif

namespace {

const char kDefaultLoader[] = "\\EFI\\BOOT\\BOOTX64.EFI";
const char kDefaultDisk[] = "/dev/sda";
const char kDefaultLabel[] = "Linux";
const int kEntryScanLimit = 0x100;

#pragma pack(push, 1)
struct MirrorVariableData {
    quint8 version;
    quint8 below4g;
    quint16 above4g;
    quint16 status;
};
#pragma pack(pop)

const quint8 kMirrorVersion = 1;
const char kMirrorCurrent[] = "MirrorCurrent";
const char kMirrorRequest[] = "MirrorRequest";

const char kEmptyLoadOption[] = {
    0x01, 0x00, 0x00, 0x00,
    0x04, 0x00,
    0x00, 0x00,
    static_cast<char>(0x7f), static_cast<char>(0xff), 0x04, 0x00
};

QString entryVarName(const QString &prefix, quint16 id)
{
    return QStringLiteral("%1%2").arg(prefix).arg(id, 4, 16, QLatin1Char('0'));
}

QList<quint16> readOrder(const QString &orderName)
{
    QList<quint16> order;
    QByteArray data = qefi_get_variable(g_efiUuid, orderName);
    const quint16 *nums = reinterpret_cast<const quint16 *>(data.constData());
    for (int i = 0; i < data.size() / 2; ++i)
        order.append(qFromLittleEndian<quint16>(nums[i]));
    return order;
}

void writeOrder(const QString &orderName, const QList<quint16> &order)
{
    QByteArray buffer(order.size() * 2, 0);
    quint16 *p = reinterpret_cast<quint16 *>(buffer.data());
    for (quint16 id : order) {
        *p = qToLittleEndian<quint16>(id);
        ++p;
    }
    qefi_set_variable(g_efiUuid, orderName, buffer);
}

bool parseHex16(const QString &text, quint16 *out)
{
    bool ok = false;
    uint value = text.toUInt(&ok, 16);
    if (!ok || value > 0xFFFF)
        return false;
    *out = static_cast<quint16>(value);
    return true;
}

quint16 findFreeEntry(const QString &prefix, const QList<quint16> &order)
{
    QSet<quint16> used;
    for (quint16 id : order)
        used.insert(id);
    for (int id = 0; id < 0x10000; ++id) {
        auto num = static_cast<quint16>(id);
        if (used.contains(num))
            continue;
        if (qefi_get_variable(g_efiUuid, entryVarName(prefix, num)).isEmpty())
            return num;
    }
    return 0xFFFF;
}

bool patchLoadOptionAttr(QByteArray *data, quint32 setBits, quint32 clearBits)
{
    if (!data || data->size() < 4)
        return false;
    quint32 attr = quint32(quint8(data->at(0))) | (quint32(quint8(data->at(1))) << 8)
        | (quint32(quint8(data->at(2))) << 16) | (quint32(quint8(data->at(3))) << 24);
    attr = (attr | setBits) & ~clearBits;
    (*data)[0] = char(attr & 0xFF);
    (*data)[1] = char((attr >> 8) & 0xFF);
    (*data)[2] = char((attr >> 16) & 0xFF);
    (*data)[3] = char((attr >> 24) & 0xFF);
    return true;
}

QString normalizeLoader(QString loader)
{
    loader.replace(QLatin1Char('/'), QLatin1Char('\\'));
    if (!loader.startsWith(QLatin1Char('\\')))
        loader.prepend(QLatin1Char('\\'));
    return loader;
}

QString expectedPartitionPath(const QString &disk, int part)
{
    QFileInfo info(disk);
    QString name = info.fileName();
    static const QRegularExpression pnumbered(QStringLiteral(R"(^(nvme|mmcblk|loop|nbd|md)\d+$)"));
    if (pnumbered.match(name).hasMatch())
        return disk + QStringLiteral("p") + QString::number(part);
    return disk + QString::number(part);
}

bool findTargetPartition(const QString &disk, int part, QEFIPartitionInfo *out)
{
    QEFIPartitionManager manager;
    QList<QEFIPartitionInfo> partitions = manager.scanPartitions();
    QString expected = expectedPartitionPath(disk, part);
    QString diskCanon = QFileInfo(disk).canonicalFilePath();
    QString expectedCanon = QFileInfo(expected).canonicalFilePath();

    for (const QEFIPartitionInfo &info : partitions) {
        if (info.devicePath == disk || info.devicePath == expected)
            return (*out = info, true);
        QString canon = QFileInfo(info.devicePath).canonicalFilePath();
        if (!diskCanon.isEmpty() && canon == diskCanon)
            return (*out = info, true);
        if (!expectedCanon.isEmpty() && canon == expectedCanon)
            return (*out = info, true);
        if (info.partitionNumber == quint32(part)
            && (info.devicePath.startsWith(disk) || disk.startsWith(info.devicePath))) {
            *out = info;
            return true;
        }
    }
    return false;
}

bool readIfaceMac(const QString &iface, quint8 mac[32], quint8 *ifType)
{
    memset(mac, 0, 32);
    *ifType = 1; // Ethernet
#ifdef Q_OS_LINUX
    QFile addrFile(QStringLiteral("/sys/class/net/%1/address").arg(iface));
    if (!addrFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QString addr = QString::fromUtf8(addrFile.readAll()).trimmed();
    const QStringList parts = addr.split(QLatin1Char(':'));
    if (parts.size() < 6)
        return false;
    for (int i = 0; i < 6 && i < parts.size(); ++i) {
        bool ok = false;
        uint byte = parts[i].toUInt(&ok, 16);
        if (!ok)
            return false;
        mac[i] = quint8(byte);
    }
    QFile typeFile(QStringLiteral("/sys/class/net/%1/type").arg(iface));
    if (typeFile.open(QIODevice::ReadOnly | QIODevice::Text))
        *ifType = quint8(typeFile.readAll().trimmed().toUInt());
    return true;
#else
    Q_UNUSED(iface);
    return false;
#endif
}

QByteArray toUcs2(const QString &text, bool terminate)
{
    QByteArray out;
    const ushort *utf16 = text.utf16();
    int count = text.size() + (terminate ? 1 : 0);
    for (int i = 0; i < count; ++i) {
        out.append(char(utf16[i] & 0xFF));
        out.append(char((utf16[i] >> 8) & 0xFF));
    }
    return out;
}

bool parseBoolFlag(const QString &text, bool *out)
{
    const QString value = text.trimmed().toLower();
    if (value == QLatin1String("t") || value == QLatin1String("true") || value == QLatin1String("1")
        || value == QLatin1String("yes")) {
        *out = true;
        return true;
    }
    if (value == QLatin1String("f") || value == QLatin1String("false") || value == QLatin1String("0")
        || value == QLatin1String("no")) {
        *out = false;
        return true;
    }
    return false;
}

quint16 parseMirrorPercent(const QString &text, bool *ok)
{
    double percent = text.trimmed().toDouble(ok);
    if (!*ok || percent < 0.0 || percent > 100.0)
        return 0;
    return quint16(qRound(percent * 100.0));
}

enum class PathStyle { HdFile, FileOnly, Edd10 };

} // namespace

// Run partition scanning test in CLI mode
static int runPartitionScanTest()
{
    std::cout << "=== EFI Partition Scanner Test ===" << std::endl;
    std::cout << "Platform: "
#ifdef Q_OS_LINUX
              << "Linux"
#elif defined(Q_OS_FREEBSD)
              << "FreeBSD"
#elif defined(Q_OS_DARWIN)
              << "macOS"
#elif defined(Q_OS_WIN)
              << "Windows"
#else
              << "Unknown"
#endif
              << std::endl;

    QEFIPartitionManager manager;
    bool hasPrivileges = manager.hasPrivileges();
    std::cout << "Running with privileges: " << (hasPrivileges ? "YES" : "NO") << std::endl;

    if (!hasPrivileges) {
        std::cerr << "WARNING: Not running with sufficient privileges" << std::endl;
        std::cerr << "Partition scanning may fail or return incomplete results" << std::endl;
    }

    std::cout << "\nScanning for partitions..." << std::endl;

    QList<QEFIPartitionInfo> partitions;
    try {
        partitions = manager.scanPartitions();
    } catch (const std::exception &e) {
        std::cerr << "ERROR: Exception during scanning: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "ERROR: Unknown exception during scanning" << std::endl;
        return 1;
    }

    std::cout << "Scan completed successfully" << std::endl;
    std::cout << "Found " << partitions.size() << " partition(s)" << std::endl;

    if (partitions.isEmpty()) {
        std::cout << "\nNo partitions found (this may be expected in test environments)" << std::endl;
        return 0;
    }

    std::cout << "\n=== Partition Details ===" << std::endl;
    for (int i = 0; i < partitions.size(); ++i) {
        const QEFIPartitionInfo &part = partitions[i];
        std::cout << "\nPartition #" << (i + 1) << ":" << std::endl;
        std::cout << "  Device Path:  " << part.devicePath.toStdString() << std::endl;
        std::cout << "  Is EFI:       " << (part.isEFI ? "YES" : "NO") << std::endl;
        std::cout << "  Partition #:  " << part.partitionNumber << std::endl;
        std::cout << "  Size:         " << (part.size / 1024 / 1024) << " MB" << std::endl;
        std::cout << "  Label:        " << part.label.toStdString() << std::endl;
        std::cout << "  Filesystem:   " << part.fileSystem.toStdString() << std::endl;
        std::cout << "  Mounted:      " << (part.isMounted ? "YES" : "NO") << std::endl;
        if (part.isMounted) {
            std::cout << "  Mount Point:  " << part.mountPoint.toStdString() << std::endl;
        }
        if (!part.partitionGuid.isNull()) {
            std::cout << "  GUID:         " << part.partitionGuid.toString().toStdString() << std::endl;
        }
    }

    std::cout << "\n=== Test PASSED ===" << std::endl;
    return 0;
}

CLI::CLI(int argc, char *argv[])
    : m_mode(EfiMode::Boot)
    , m_argc(argc)
    , m_argv(argv)
{
    m_app = new QCoreApplication(m_argc, m_argv);
    setupParser();
}

CLI::~CLI()
{
    delete m_app;
}

QString CLI::prefix() const
{
    switch (m_mode) {
    case EfiMode::Driver:
        return QStringLiteral("Driver");
    case EfiMode::SysPrep:
        return QStringLiteral("SysPrep");
    case EfiMode::Boot:
    default:
        return QStringLiteral("Boot");
    }
}

QString CLI::orderName() const
{
    switch (m_mode) {
    case EfiMode::Driver:
        return QStringLiteral("DriverOrder");
    case EfiMode::SysPrep:
        return QStringLiteral("SysPrepOrder");
    case EfiMode::Boot:
    default:
        return QStringLiteral("BootOrder");
    }
}

void CLI::setupParser()
{
    m_parser.setApplicationDescription("EFI Boot Manager - A userspace cross-platform EFI boot entry management CLI");
    m_parser.addHelpOption();
    m_parser.addPositionalArgument("extra", "Optional extra arguments passed to the loader", "[args...]");

    m_parser.addOption(QCommandLineOption({"v", "verbose"}, "Verbose mode - prints additional information"));
    m_parser.addOption(QCommandLineOption({"q", "quiet"}, "Quiet mode - suppresses output"));
    m_parser.addOption(QCommandLineOption({"V", "version"}, "Print version string and exit"));

    m_parser.addOption(QCommandLineOption({"b", "bootnum"}, "Modify BootXXXX (hex)", "XXXX"));
    m_parser.addOption(QCommandLineOption({"a", "active"}, "Set bootnum active"));
    m_parser.addOption(QCommandLineOption({"A", "inactive"}, "Set bootnum inactive"));
    m_parser.addOption(QCommandLineOption({"B", "delete-bootnum"}, "Delete bootnum"));

    m_parser.addOption(QCommandLineOption({"o", "bootorder"}, "Explicitly set BootOrder (hex, comma-separated)", "XXXX,YYYY,ZZZZ"));
    m_parser.addOption(QCommandLineOption({"O", "delete-bootorder"}, "Delete BootOrder"));
    m_parser.addOption(QCommandLineOption({"n", "bootnext"}, "Set BootNext to XXXX (hex)", "XXXX"));
    m_parser.addOption(QCommandLineOption({"N", "delete-bootnext"}, "Delete BootNext"));

    m_parser.addOption(QCommandLineOption({"t", "timeout"}, "Boot Manager timeout, in seconds", "seconds"));
    m_parser.addOption(QCommandLineOption({"T", "delete-timeout"}, "Delete Timeout variable"));

    m_parser.addOption(QCommandLineOption({"c", "create"}, "Create new variable bootnum and add to bootorder at index (-I)"));
    m_parser.addOption(QCommandLineOption({"C", "create-only"}, "Create new variable bootnum and do not add to bootorder"));
    m_parser.addOption(QCommandLineOption({"d", "disk"}, "Disk containing boot loader (defaults to /dev/sda)", "DISK"));
    m_parser.addOption(QCommandLineOption({"p", "part"}, "Partition containing loader (defaults to 1)", "PART"));
    m_parser.addOption(QCommandLineOption({"l", "loader"}, "Loader path (defaults to \\EFI\\BOOT\\BOOTX64.EFI)", "NAME"));
    m_parser.addOption(QCommandLineOption({"L", "label"}, "Boot manager display label (defaults to Linux)", "LABEL"));
    m_parser.addOption(QCommandLineOption({"I", "index"}, "When creating an entry, insert it in bootorder at specified position (default: 0)", "number"));

    m_parser.addOption(QCommandLineOption({"D", "remove-dups"}, "Remove duplicated entries from BootOrder"));
    m_parser.addOption(QCommandLineOption({"e", "edd"}, "Force EDD 1.0 or 3.0 creation variables, or guess (-1)", "1|3|-1"));
    m_parser.addOption(QCommandLineOption({"E", "device", "edd-device"}, "EDD 1.0 device number (defaults to 0x80)", "num"));
    m_parser.addOption(QCommandLineOption("full-dev-path", "Use a full device path"));
    m_parser.addOption(QCommandLineOption("file-dev-path", "Use an abbreviated File() device path"));
    m_parser.addOption(QCommandLineOption({"f", "reconnect"}, "Re-connect devices after driver is loaded"));
    m_parser.addOption(QCommandLineOption({"F", "no-reconnect"}, "Do not re-connect devices after driver is loaded"));
    m_parser.addOption(QCommandLineOption({"g", "gpt"}, "Force disk with invalid PMBR to be treated as GPT"));
    m_parser.addOption(QCommandLineOption({"i", "iface"}, "Create a netboot entry for the named interface", "name"));
    m_parser.addOption(QCommandLineOption({"m", "mirror-below-4G"}, "Mirror memory below 4GB (t|f)", "t|f"));
    m_parser.addOption(QCommandLineOption({"M", "mirror-above-4G"}, "Percentage memory to mirror above 4GB", "X"));
    m_parser.addOption(QCommandLineOption({"r", "driver"}, "Operate on Driver variables, not Boot Variables"));
    m_parser.addOption(QCommandLineOption({"u", "unicode", "UCS-2"}, "Handle extra args as UCS-2 (default is ASCII)"));
    m_parser.addOption(QCommandLineOption({"w", "write-signature"}, "Write unique sig to MBR if needed"));
    m_parser.addOption(QCommandLineOption({"y", "sysprep"}, "Operate on SysPrep variables, not Boot Variables"));
    m_parser.addOption(QCommandLineOption({"@", "append-binary-args"}, "Append extra args from file (use - for stdin)", "file"));

#ifndef QT_NO_DEBUG
    m_parser.addOption(QCommandLineOption("test-scan", "Test EFI partition scanning (CLI mode for testing)"));
#endif
}

void CLI::printVersion()
{
    QTextStream out(stdout);
    out << "QEFIEntryManager version " << QEFI_ENTRY_MANAGER_VERSION << Qt::endl;
    out << "efibootmgr compatible CLI mode" << Qt::endl;
}

void CLI::printMirror()
{
    QTextStream out(stdout);
    auto printOne = [&](const char *name, const char *aboveLabel, const char *belowLabel) {
        QByteArray data = qefi_get_variable(g_mirrorUuid, QString::fromLatin1(name));
        if (data.size() < int(sizeof(MirrorVariableData)))
            return false;
        MirrorVariableData mirror {};
        memcpy(&mirror, data.constData(), sizeof(mirror));
        quint16 above = qFromLittleEndian<quint16>(mirror.above4g);
        out << aboveLabel << ": " << (above / 100) << "."
            << QString::number(above % 100).rightJustified(2, QLatin1Char('0')) << Qt::endl;
        out << belowLabel << ": " << (mirror.below4g ? "true" : "false") << Qt::endl;
        return true;
    };
    printOne(kMirrorCurrent, "MirroredPercentageAbove4G", "MirrorMemoryBelow4GB");
    printOne(kMirrorRequest, "RequestMirroredPercentageAbove4G", "RequestMirrorMemoryBelow4GB");
}

void CLI::printEntries(bool verbose, bool quiet)
{
    if (quiet)
        return;

    QTextStream out(stdout);
    const QString pref = prefix();
    const QString orderVar = orderName();

    if (m_mode == EfiMode::Boot) {
        QByteArray nextData = qefi_get_variable(g_efiUuid, QStringLiteral("BootNext"));
        if (nextData.size() >= 2) {
            quint16 next = qefi_get_variable_uint16(g_efiUuid, QStringLiteral("BootNext"));
            out << "BootNext: " << QString::number(next, 16).rightJustified(4, QLatin1Char('0')).toUpper() << Qt::endl;
        }

        QByteArray currentData = qefi_get_variable(g_efiUuid, QStringLiteral("BootCurrent"));
        if (currentData.size() >= 2) {
            quint16 current = qefi_get_variable_uint16(g_efiUuid, QStringLiteral("BootCurrent"));
            out << "BootCurrent: " << QString::number(current, 16).rightJustified(4, QLatin1Char('0')).toUpper() << Qt::endl;
        }

        QByteArray timeoutData = qefi_get_variable(g_efiUuid, QStringLiteral("Timeout"));
        if (timeoutData.size() >= 2) {
            quint16 timeout = qefi_get_variable_uint16(g_efiUuid, QStringLiteral("Timeout"));
            out << "Timeout: " << timeout << " seconds" << Qt::endl;
        }
    }

    QList<quint16> order = readOrder(orderVar);
    if (order.isEmpty()) {
        if (m_mode == EfiMode::Boot)
            out << "No BootOrder is set; firmware will attempt recovery" << Qt::endl;
        else
            out << "No " << orderVar << " is set" << Qt::endl;
    } else {
        out << orderVar << ": ";
        for (int i = 0; i < order.size(); ++i) {
            out << QString::number(order[i], 16).rightJustified(4, QLatin1Char('0')).toUpper();
            if (i < order.size() - 1)
                out << ",";
        }
        out << Qt::endl;
    }

    QMap<quint16, QByteArray> entries;
    auto consider = [&](quint16 id) {
        if (entries.contains(id))
            return;
        QByteArray data = qefi_get_variable(g_efiUuid, entryVarName(pref, id));
        if (!data.isEmpty())
            entries.insert(id, data);
    };
    for (quint16 id : std::as_const(order))
        consider(id);
    for (int id = 0; id < kEntryScanLimit; ++id)
        consider(quint16(id));

    for (auto it = entries.constBegin(); it != entries.constEnd(); ++it) {
        QEFILoadOption option(it.value());
        out << pref << QString::number(it.key(), 16).rightJustified(4, QLatin1Char('0')).toUpper();
        out << (option.isVisible() ? "*" : " ");
        out << " " << option.name();
        if (verbose)
            out << "\t" << option.path();
        out << Qt::endl;
    }

    if (m_mode == EfiMode::Boot)
        printMirror();
}

QByteArray CLI::collectOptionalData(QTextStream &err, bool *ok)
{
    *ok = true;
    if (m_parser.isSet("append-binary-args")) {
        QString path = m_parser.value("append-binary-args");
        QFile file;
        if (path == QLatin1String("-")) {
            file.open(stdin, QIODevice::ReadOnly);
        } else {
            file.setFileName(path);
            if (!file.open(QIODevice::ReadOnly)) {
                err << "Error: Could not read extra args from " << path << Qt::endl;
                *ok = false;
                return {};
            }
        }
        return file.readAll();
    }

    const QStringList extra = m_parser.positionalArguments();
    if (extra.isEmpty())
        return {};

    if (m_parser.isSet("unicode")) {
        QByteArray data;
        for (int i = 0; i < extra.size(); ++i) {
            data.append(toUcs2(extra[i], false));
            if (i + 1 < extra.size())
                data.append("\0\0", 2);
        }
        data.append("\0\0", 2);
        return data;
    }

    return extra.join(QLatin1Char(' ')).toUtf8();
}

bool CLI::createLoadOption(QByteArray *out, QTextStream &err)
{
    QByteArray seed(kEmptyLoadOption, int(sizeof(kEmptyLoadOption)));
    QEFILoadOption option(seed);
    while (!option.devicePathList().isEmpty())
        option.removeDevicePathAt(0);

    const QString label = m_parser.isSet("label") ? m_parser.value("label") : QString::fromLatin1(kDefaultLabel);
    option.setName(label);
    option.setIsVisible(true);

    bool extraOk = true;
    QByteArray optional = collectOptionalData(err, &extraOk);
    if (!extraOk)
        return false;
    if (!optional.isEmpty())
        option.setOptionalData(optional);

    PathStyle style = PathStyle::HdFile;
    if (m_parser.isSet("file-dev-path"))
        style = PathStyle::FileOnly;
    if (m_parser.isSet("full-dev-path"))
        style = PathStyle::HdFile;
    if (m_parser.isSet("edd")) {
        const QString edd = m_parser.value("edd");
        if (edd == QLatin1String("1"))
            style = PathStyle::Edd10;
        else if (edd == QLatin1String("3") || edd == QLatin1String("-1"))
            style = PathStyle::HdFile;
        else {
            err << "Error: --edd must be 1, 3, or -1" << Qt::endl;
            return false;
        }
    }

    const QString loader = normalizeLoader(
        m_parser.isSet("loader") ? m_parser.value("loader") : QString::fromLatin1(kDefaultLoader));

    if (m_parser.isSet("iface")) {
        quint8 mac[32];
        quint8 ifType = 1;
        if (!readIfaceMac(m_parser.value("iface"), mac, &ifType)) {
            err << "Error: Could not read MAC address for interface " << m_parser.value("iface") << Qt::endl;
            return false;
        }
        option.addDevicePath(new QEFIDevicePathMessageMACAddr(mac, ifType));
    } else if (style == PathStyle::FileOnly) {
        option.addDevicePath(new QEFIDevicePathMediaFile(loader));
    } else if (style == PathStyle::Edd10) {
        quint16 device = 0x80;
        if (m_parser.isSet("device")) {
            bool ok = false;
            uint parsed = m_parser.value("device").toUInt(&ok, 0);
            if (!ok) {
                err << "Error: Invalid --device number" << Qt::endl;
                return false;
            }
            device = quint16(parsed);
        }
        QByteArray description;
        description.append(char(device & 0xFF));
        option.addDevicePath(new QEFIDevicePathBIOSBoot(0x02, 0, description));
        option.addDevicePath(new QEFIDevicePathMediaFile(loader));
    } else {
        const QString disk = m_parser.isSet("disk") ? m_parser.value("disk") : QString::fromLatin1(kDefaultDisk);
        int part = 1;
        if (m_parser.isSet("part")) {
            bool ok = false;
            part = m_parser.value("part").toInt(&ok);
            if (!ok || part < 0) {
                err << "Error: Invalid --part number" << Qt::endl;
                return false;
            }
        }

        QEFIPartitionInfo partition {};
        bool found = findTargetPartition(disk, part, &partition);
        quint8 signature[16] = {0};
        quint8 format = m_parser.isSet("gpt")
            ? quint8(QEFIDevicePathMediaHD::QEFIDevicePathMediaHDFormat::GPT)
            : quint8(QEFIDevicePathMediaHD::QEFIDevicePathMediaHDFormat::GPT);
        quint8 signatureType = QEFIDevicePathMediaHD::QEFIDevicePathMediaHDSignatureType::NONE;
        quint32 partitionNumber = found ? partition.partitionNumber : quint32(part);
        quint64 start = found ? partition.startLba : 0;
        quint64 sizeSectors = found && partition.size ? partition.size / 512 : 0;

        if (found && !partition.partitionGuid.isNull()) {
            QByteArray guidBytes = qefi_rfc4122_to_guid(partition.partitionGuid.toRfc4122());
            if (guidBytes.size() == 16)
                memcpy(signature, guidBytes.constData(), 16);
            format = QEFIDevicePathMediaHD::QEFIDevicePathMediaHDFormat::GPT;
            signatureType = QEFIDevicePathMediaHD::QEFIDevicePathMediaHDSignatureType::GUID;
        } else if (m_parser.isSet("write-signature") || m_parser.isSet("gpt")) {
            quint32 mbrSig = QRandomGenerator::global()->generate();
            memcpy(signature, &mbrSig, 4);
            if (m_parser.isSet("gpt")) {
                format = QEFIDevicePathMediaHD::QEFIDevicePathMediaHDFormat::GPT;
                signatureType = QEFIDevicePathMediaHD::QEFIDevicePathMediaHDSignatureType::GUID;
            } else {
                format = QEFIDevicePathMediaHD::QEFIDevicePathMediaHDFormat::PCAT;
                signatureType = QEFIDevicePathMediaHD::QEFIDevicePathMediaHDSignatureType::MBR;
            }
            if (m_parser.isSet("write-signature")) {
                QFile diskFile(disk);
                if (diskFile.open(QIODevice::ReadWrite)) {
                    diskFile.seek(440);
                    diskFile.write(reinterpret_cast<const char *>(&mbrSig), 4);
                }
            }
        }

        option.addDevicePath(new QEFIDevicePathMediaHD(partitionNumber, start, sizeSectors, signature, format, signatureType));
        option.addDevicePath(new QEFIDevicePathMediaFile(loader));
    }

    QByteArray formatted = option.format();
    if (formatted.isEmpty()) {
        err << "Error: Failed to format boot entry load option" << Qt::endl;
        return false;
    }

    if (m_parser.isSet("reconnect"))
        patchLoadOptionAttr(&formatted, QEFI_LOAD_OPTION_FORCE_RECONNECT, 0);
    if (m_parser.isSet("no-reconnect"))
        patchLoadOptionAttr(&formatted, 0, QEFI_LOAD_OPTION_FORCE_RECONNECT);

    *out = formatted;
    return true;
}

bool CLI::handleCreate(QTextStream &out, QTextStream &err, bool quiet)
{
    if (m_parser.isSet("create") && m_parser.isSet("create-only")) {
        err << "Error: Cannot specify both --create and --create-only" << Qt::endl;
        return false;
    }

    const QString pref = prefix();
    QList<quint16> order = readOrder(orderName());
    quint16 id = 0;
    if (m_parser.isSet("bootnum")) {
        if (!parseHex16(m_parser.value("bootnum"), &id)) {
            err << "Error: Invalid bootnum format" << Qt::endl;
            return false;
        }
        if (!qefi_get_variable(g_efiUuid, entryVarName(pref, id)).isEmpty()) {
            err << "Error: Cannot create " << pref
                << QString::number(id, 16).rightJustified(4, QLatin1Char('0')).toUpper()
                << ": already exists." << Qt::endl;
            return false;
        }
    } else {
        id = findFreeEntry(pref, order);
        if (id == 0xFFFF) {
            err << "Error: no available " << pref << " variables" << Qt::endl;
            return false;
        }
    }

    QByteArray data;
    if (!createLoadOption(&data, err))
        return false;

    qefi_set_variable(g_efiUuid, entryVarName(pref, id), data);

    if (m_parser.isSet("create")) {
        int index = 0;
        if (m_parser.isSet("index")) {
            bool ok = false;
            index = m_parser.value("index").toInt(&ok);
            if (!ok || index < 0) {
                err << "Error: Invalid --index" << Qt::endl;
                return false;
            }
        }
        if (index > order.size())
            index = order.size();
        order.insert(index, id);
        writeOrder(orderName(), order);
    }

    if (!quiet) {
        out << "Created " << pref << QString::number(id, 16).rightJustified(4, QLatin1Char('0')).toUpper()
            << Qt::endl;
    }
    return true;
}

bool CLI::handleMirror(QTextStream &out, QTextStream &err, bool quiet)
{
    bool below = false;
    quint16 above = 0;
    QByteArray existing = qefi_get_variable(g_mirrorUuid, QString::fromLatin1(kMirrorRequest));
    if (existing.size() < int(sizeof(MirrorVariableData)))
        existing = qefi_get_variable(g_mirrorUuid, QString::fromLatin1(kMirrorCurrent));
    MirrorVariableData mirror {};
    mirror.version = kMirrorVersion;
    if (existing.size() >= int(sizeof(MirrorVariableData)))
        memcpy(&mirror, existing.constData(), sizeof(mirror));

    if (m_parser.isSet("mirror-below-4G")) {
        if (!parseBoolFlag(m_parser.value("mirror-below-4G"), &below)) {
            err << "Error: --mirror-below-4G expects t|f" << Qt::endl;
            return false;
        }
        mirror.below4g = below ? 1 : 0;
    }
    if (m_parser.isSet("mirror-above-4G")) {
        bool ok = false;
        above = parseMirrorPercent(m_parser.value("mirror-above-4G"), &ok);
        if (!ok) {
            err << "Error: Invalid --mirror-above-4G percentage" << Qt::endl;
            return false;
        }
        mirror.above4g = qToLittleEndian<quint16>(above);
    }
    mirror.version = kMirrorVersion;
    mirror.status = 0;

    QByteArray data(reinterpret_cast<const char *>(&mirror), int(sizeof(mirror)));
    qefi_set_variable(g_mirrorUuid, QString::fromLatin1(kMirrorRequest), data);
    if (!quiet)
        out << "Mirror request updated" << Qt::endl;
    return true;
}

bool CLI::handleModifications()
{
    bool quiet = m_parser.isSet("quiet");
    QTextStream out(stdout);
    QTextStream err(stderr);
    const QString pref = prefix();
    const QString orderVar = orderName();

    if (m_parser.isSet("index") && !m_parser.isSet("create")) {
        err << "Error: Index is meaningless without create" << Qt::endl;
        return false;
    }

    if ((m_parser.isSet("reconnect") || m_parser.isSet("no-reconnect")) && m_mode != EfiMode::Driver
        && !m_parser.isSet("create") && !m_parser.isSet("create-only")) {
        err << "Error: --reconnect is supported only for driver entries." << Qt::endl;
        return false;
    }
    if (m_parser.isSet("reconnect") && m_parser.isSet("no-reconnect")) {
        err << "Error: Cannot specify both --reconnect and --no-reconnect" << Qt::endl;
        return false;
    }

    if (m_mode != EfiMode::Boot && (m_parser.isSet("bootnext") || m_parser.isSet("delete-bootnext"))) {
        err << "Error: " << (m_mode == EfiMode::SysPrep ? "--sysprep" : "--driver")
            << " mode does not support BootNext options." << Qt::endl;
        return false;
    }
    if (m_mode != EfiMode::Boot && (m_parser.isSet("timeout") || m_parser.isSet("delete-timeout"))) {
        err << "Error: " << (m_mode == EfiMode::SysPrep ? "--sysprep" : "--driver")
            << " mode does not support timeout options." << Qt::endl;
        return false;
    }

    if (m_parser.isSet("delete-bootnum")) {
        if (!m_parser.isSet("bootnum")) {
            err << "Error: --delete-bootnum requires --bootnum" << Qt::endl;
            return false;
        }
        quint16 bootnum = 0;
        if (!parseHex16(m_parser.value("bootnum"), &bootnum)) {
            err << "Error: Invalid bootnum format" << Qt::endl;
            return false;
        }
        qefi_set_variable(g_efiUuid, entryVarName(pref, bootnum), QByteArray());
        QList<quint16> order = readOrder(orderVar);
        order.removeAll(bootnum);
        writeOrder(orderVar, order);
        if (!quiet)
            out << "Deleted " << pref << QString::number(bootnum, 16).rightJustified(4, QLatin1Char('0')).toUpper() << Qt::endl;
    }

    if (m_parser.isSet("active") || m_parser.isSet("inactive")) {
        if (!m_parser.isSet("bootnum")) {
            err << "Error: --active/--inactive requires --bootnum" << Qt::endl;
            return false;
        }
        if (m_parser.isSet("active") && m_parser.isSet("inactive")) {
            err << "Error: Cannot specify both --active and --inactive" << Qt::endl;
            return false;
        }
        quint16 bootnum = 0;
        if (!parseHex16(m_parser.value("bootnum"), &bootnum)) {
            err << "Error: Invalid bootnum format" << Qt::endl;
            return false;
        }
        QByteArray data = qefi_get_variable(g_efiUuid, entryVarName(pref, bootnum));
        bool visible = m_parser.isSet("active");
        if (!patchLoadOptionAttr(&data, visible ? QEFI_LOAD_OPTION_ACTIVE : 0,
                                 visible ? 0 : QEFI_LOAD_OPTION_ACTIVE)) {
            err << "Error: Failed to set boot entry visibility" << Qt::endl;
            return false;
        }
        qefi_set_variable(g_efiUuid, entryVarName(pref, bootnum), data);
        if (!quiet) {
            out << "Set " << pref << QString::number(bootnum, 16).rightJustified(4, QLatin1Char('0')).toUpper()
                << (visible ? " active" : " inactive") << Qt::endl;
        }
    }

    if (m_parser.isSet("reconnect") || m_parser.isSet("no-reconnect")) {
        if (!m_parser.isSet("create") && !m_parser.isSet("create-only")) {
            if (!m_parser.isSet("bootnum")) {
                err << "Error: --reconnect requires --bootnum" << Qt::endl;
                return false;
            }
            quint16 bootnum = 0;
            if (!parseHex16(m_parser.value("bootnum"), &bootnum)) {
                err << "Error: Invalid bootnum format" << Qt::endl;
                return false;
            }
            QByteArray data = qefi_get_variable(g_efiUuid, entryVarName(pref, bootnum));
            bool reconnect = m_parser.isSet("reconnect");
            if (!patchLoadOptionAttr(&data, reconnect ? QEFI_LOAD_OPTION_FORCE_RECONNECT : 0,
                                     reconnect ? 0 : QEFI_LOAD_OPTION_FORCE_RECONNECT)) {
                err << "Error: Failed to set re-connect" << Qt::endl;
                return false;
            }
            qefi_set_variable(g_efiUuid, entryVarName(pref, bootnum), data);
        }
    }

    if (m_parser.isSet("bootorder")) {
        QStringList orderParts = m_parser.value("bootorder").split(QLatin1Char(','));
        QList<quint16> newOrder;
        for (const QString &part : orderParts) {
            quint16 id = 0;
            if (!parseHex16(part.trimmed(), &id)) {
                err << "Error: Invalid bootorder format" << Qt::endl;
                return false;
            }
            newOrder.append(id);
        }
        writeOrder(orderVar, newOrder);
        if (!quiet)
            out << orderVar << " set to: " << m_parser.value("bootorder") << Qt::endl;
    }

    if (m_parser.isSet("delete-bootorder")) {
        writeOrder(orderVar, {});
        if (!quiet)
            out << orderVar << " deleted" << Qt::endl;
    }

    if (m_parser.isSet("bootnext")) {
        quint16 bootnext = 0;
        if (!parseHex16(m_parser.value("bootnext"), &bootnext)) {
            err << "Error: Invalid bootnext format" << Qt::endl;
            return false;
        }
        qefi_set_variable_uint16(g_efiUuid, QStringLiteral("BootNext"), bootnext);
        if (!quiet)
            out << "BootNext set to: " << QString::number(bootnext, 16).rightJustified(4, QLatin1Char('0')).toUpper() << Qt::endl;
    }

    if (m_parser.isSet("delete-bootnext")) {
        qefi_set_variable(g_efiUuid, QStringLiteral("BootNext"), QByteArray());
        if (!quiet)
            out << "BootNext deleted" << Qt::endl;
    }

    if (m_parser.isSet("timeout")) {
        bool ok = false;
        uint timeout = m_parser.value("timeout").toUInt(&ok);
        if (!ok || timeout > 0xFFFF) {
            err << "Error: Invalid timeout format" << Qt::endl;
            return false;
        }
        qefi_set_variable_uint16(g_efiUuid, QStringLiteral("Timeout"), quint16(timeout));
        if (!quiet)
            out << "Timeout set to: " << timeout << " seconds" << Qt::endl;
    }

    if (m_parser.isSet("delete-timeout")) {
        qefi_set_variable(g_efiUuid, QStringLiteral("Timeout"), QByteArray());
        if (!quiet)
            out << "Timeout deleted" << Qt::endl;
    }

    if (m_parser.isSet("remove-dups")) {
        QList<quint16> order = readOrder(orderVar);
        QList<quint16> uniqueOrder;
        for (quint16 id : order) {
            if (!uniqueOrder.contains(id))
                uniqueOrder.append(id);
        }
        if (uniqueOrder.size() != order.size()) {
            writeOrder(orderVar, uniqueOrder);
            if (!quiet)
                out << "Removed duplicate entries from " << orderVar << Qt::endl;
        } else if (!quiet) {
            out << "No duplicate entries found in " << orderVar << Qt::endl;
        }
    }

    if (m_parser.isSet("create") || m_parser.isSet("create-only")) {
        if (!handleCreate(out, err, quiet))
            return false;
    }

    if (m_parser.isSet("mirror-below-4G") || m_parser.isSet("mirror-above-4G")) {
        if (!handleMirror(out, err, quiet))
            return false;
    }

    return true;
}

int CLI::execute()
{
    m_parser.process(*m_app);

    if (m_parser.isSet("version")) {
        printVersion();
        return 0;
    }

    if (!qefi_has_privilege()) {
        QTextStream err(stderr);
        err << "Error: Insufficient privileges. Please run as root or with elevated privileges." << Qt::endl;
        return 1;
    }

#ifndef QT_NO_DEBUG
    if (m_parser.isSet("test-scan")) {
        return runPartitionScanTest();
    }
#endif

    if (!qefi_is_available()) {
        QTextStream err(stderr);
        err << "Error: No EFI environment available" << Qt::endl;
        return 1;
    }

    if (m_parser.isSet("driver") && m_parser.isSet("sysprep")) {
        QTextStream err(stderr);
        err << "Error: --sysprep and --driver may not be used together." << Qt::endl;
        return 1;
    }
    if (m_parser.isSet("driver"))
        m_mode = EfiMode::Driver;
    else if (m_parser.isSet("sysprep"))
        m_mode = EfiMode::SysPrep;

    QEFIEntryStaticList *list = QEFIEntryStaticList::instance();
    list->load();

    bool hasModifications = m_parser.isSet("active") || m_parser.isSet("inactive") || m_parser.isSet("delete-bootnum")
        || m_parser.isSet("bootorder") || m_parser.isSet("delete-bootorder") || m_parser.isSet("bootnext")
        || m_parser.isSet("delete-bootnext") || m_parser.isSet("timeout") || m_parser.isSet("delete-timeout")
        || m_parser.isSet("remove-dups") || m_parser.isSet("create") || m_parser.isSet("create-only")
        || m_parser.isSet("reconnect") || m_parser.isSet("no-reconnect") || m_parser.isSet("mirror-below-4G")
        || m_parser.isSet("mirror-above-4G") || m_parser.isSet("index");

    if (hasModifications) {
        if (!handleModifications())
            return 1;
        list->load();
    }

    printEntries(m_parser.isSet("verbose"), m_parser.isSet("quiet"));
    return 0;
}
