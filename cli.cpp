#include "cli.h"
#include "qefi.h"
#include "qefientry.h"
#include "qefientrystaticlist.h"
#include "qefipartitionmanager.h"
#include "version.h"

#include <QDebug>
#include <QTextStream>
#include <QtEndian>
#include <iostream>

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#define hex Qt::hex
#define dec Qt::dec
#endif

// Defined in UEFI Spec as "EFI_GLOBAL_VARIABLE"
constexpr QUuid g_efiUuid = QUuid(0x8be4df61, 0x93ca, 0x11d2, 0xaa, 0x0d, 0x00, 0xe0, 0x98, 0x03, 0x2b, 0x8c);

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

    // Check privileges
    QEFIPartitionManager manager;
    bool hasPrivileges = manager.hasPrivileges();
    std::cout << "Running with privileges: " << (hasPrivileges ? "YES" : "NO") << std::endl;

    if (!hasPrivileges) {
        std::cerr << "WARNING: Not running with sufficient privileges" << std::endl;
        std::cerr << "Partition scanning may fail or return incomplete results" << std::endl;
    }

    std::cout << "\nScanning for partitions..." << std::endl;

    // Perform scan
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

    // Display partition information
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
    : m_argc(argc)
    , m_argv(argv)
{
    m_app = new QCoreApplication(m_argc, m_argv);
    setupParser();
}

CLI::~CLI()
{
    delete m_app;
}

void CLI::setupParser()
{
    m_parser.setApplicationDescription("EFI Boot Manager - A userspace cross-platform EFI boot entry management CLI");
    m_parser.addHelpOption();

    // Display options
    m_parser.addOption(QCommandLineOption({"v", "verbose"}, "Verbose mode - prints additional information"));
    m_parser.addOption(QCommandLineOption({"q", "quiet"}, "Quiet mode - suppresses output"));
    m_parser.addOption(QCommandLineOption({"V", "version"}, "Print version string and exit"));

    // Boot entry selection
    m_parser.addOption(QCommandLineOption({"b", "bootnum"}, "Modify BootXXXX (hex)", "XXXX"));

    // Boot entry modification
    m_parser.addOption(QCommandLineOption({"a", "active"}, "Set bootnum active"));
    m_parser.addOption(QCommandLineOption({"A", "inactive"}, "Set bootnum inactive"));
    m_parser.addOption(QCommandLineOption({"B", "delete-bootnum"}, "Delete bootnum"));

    // Boot order management
    m_parser.addOption(QCommandLineOption({"o", "bootorder"}, "Explicitly set BootOrder (hex, comma-separated)", "XXXX,YYYY,ZZZZ"));
    m_parser.addOption(QCommandLineOption({"O", "delete-bootorder"}, "Delete BootOrder"));
    m_parser.addOption(QCommandLineOption({"n", "bootnext"}, "Set BootNext to XXXX (hex)", "XXXX"));
    m_parser.addOption(QCommandLineOption({"N", "delete-bootnext"}, "Delete BootNext"));

    // Timeout
    m_parser.addOption(QCommandLineOption({"t", "timeout"}, "Boot Manager timeout, in seconds", "seconds"));
    m_parser.addOption(QCommandLineOption({"T", "delete-timeout"}, "Delete Timeout variable"));

    // Boot entry creation
    m_parser.addOption(QCommandLineOption({"c", "create"}, "Create new variable bootnum and add to bootorder"));
    m_parser.addOption(QCommandLineOption({"C", "create-only"}, "Create new variable bootnum and do not add to bootorder"));
    m_parser.addOption(QCommandLineOption({"d", "disk"}, "The disk containing the loader (defaults to /dev/sda)", "DISK"));
    m_parser.addOption(QCommandLineOption({"p", "part"}, "Partition number containing the bootloader (defaults to 1)", "PART"));
    m_parser.addOption(QCommandLineOption({"l", "loader"}, "Specify a loader (defaults to \\\\EFI\\\\BOOT\\\\BOOTX64.EFI)", "NAME"));
    m_parser.addOption(QCommandLineOption({"L", "label"}, "Boot manager display label (defaults to 'Linux')", "LABEL"));

    // Additional options
    m_parser.addOption(QCommandLineOption({"D", "remove-dups"}, "Remove duplicated entries from BootOrder"));

#ifndef QT_NO_DEBUG
    // Test scan option
    m_parser.addOption(QCommandLineOption("test-scan", "Test EFI partition scanning (CLI mode for testing)"));
#endif
}

void CLI::printVersion()
{
    QTextStream out(stdout);
    out << "QEFIEntryManager version " << QEFI_ENTRY_MANAGER_VERSION << Qt::endl;
    out << "efibootmgr compatible CLI mode" << Qt::endl;
}

void CLI::printBootEntries(bool verbose, bool quiet)
{
    if (quiet)
        return;

    QTextStream out(stdout);
    QEFIEntryStaticList *list = QEFIEntryStaticList::instance();

    // Get current boot entry
    quint16 current = qefi_get_variable_uint16(g_efiUuid, QStringLiteral("BootCurrent"));

    // Get BootNext if exists
    quint16 next = qefi_get_variable_uint16(g_efiUuid, QStringLiteral("BootNext"));

    // Print timeout
    out << "Timeout: " << list->timeout() << " seconds" << Qt::endl;

    // Print boot order
    out << "BootOrder: ";
    QList<quint16> order = list->order();
    for (int i = 0; i < order.size(); i++) {
        out << QString::number(order[i], 16).rightJustified(4, '0').toUpper();
        if (i < order.size() - 1) {
            out << ",";
        }
    }
    out << Qt::endl;

    // Print current boot entry
    if (current != 0xFFFF) {
        out << "BootCurrent: " << QString::number(current, 16).rightJustified(4, '0').toUpper() << Qt::endl;
    }

    // Print BootNext if set
    if (next != 0xFFFF) {
        out << "BootNext: " << QString::number(next, 16).rightJustified(4, '0').toUpper() << Qt::endl;
    }

    // Print boot entries
    QMap<quint16, QEFIEntry> entries = list->entries();
    for (auto it = entries.constBegin(); it != entries.constEnd(); ++it) {
        quint16 id = it.key();
        const QEFIEntry &entry = it.value();

        out << "Boot" << QString::number(id, 16).rightJustified(4, '0').toUpper();
        if (entry.isActive()) {
            out << "*";
        } else {
            out << " ";
        }
        out << " " << entry.name();

        if (verbose) {
            out << "\t" << entry.devicePath();
        }

        out << Qt::endl;
    }
}

bool CLI::handleModifications()
{
    QEFIEntryStaticList *list = QEFIEntryStaticList::instance();
    bool modified = false;
    bool quiet = m_parser.isSet("quiet");
    QTextStream out(stdout);
    QTextStream err(stderr);

    // Handle delete bootnum
    if (m_parser.isSet("delete-bootnum")) {
        if (!m_parser.isSet("bootnum")) {
            err << "Error: --delete-bootnum requires --bootnum" << Qt::endl;
            return false;
        }

        QString bootnumStr = m_parser.value("bootnum");
        bool ok;
        quint16 bootnum = bootnumStr.toUShort(&ok, 16);
        if (!ok) {
            err << "Error: Invalid bootnum format" << Qt::endl;
            return false;
        }

        // Delete by setting empty data
        QString name = QStringLiteral("Boot%1").arg(bootnum, 4, 16, QLatin1Char('0'));
        qefi_set_variable(g_efiUuid, name, QByteArray());

        // Remove from boot order
        QList<quint16> order = list->order();
        order.removeAll(bootnum);
        list->setBootOrder(order);

        if (!quiet) {
            out << "Deleted Boot" << QString::number(bootnum, 16).rightJustified(4, '0').toUpper() << Qt::endl;
        }
        modified = true;
    }

    // Handle active/inactive
    if (m_parser.isSet("active") || m_parser.isSet("inactive")) {
        if (!m_parser.isSet("bootnum")) {
            err << "Error: --active/--inactive requires --bootnum" << Qt::endl;
            return false;
        }

        if (m_parser.isSet("active") && m_parser.isSet("inactive")) {
            err << "Error: Cannot specify both --active and --inactive" << Qt::endl;
            return false;
        }

        QString bootnumStr = m_parser.value("bootnum");
        bool ok;
        quint16 bootnum = bootnumStr.toUShort(&ok, 16);
        if (!ok) {
            err << "Error: Invalid bootnum format" << Qt::endl;
            return false;
        }

        bool visible = m_parser.isSet("active");
        if (list->setBootVisibility(bootnum, visible)) {
            if (!quiet) {
                out << "Set Boot" << QString::number(bootnum, 16).rightJustified(4, '0').toUpper() << (visible ? " active" : " inactive") << Qt::endl;
            }
            modified = true;
        } else {
            err << "Error: Failed to set boot entry visibility" << Qt::endl;
            return false;
        }
    }

    // Handle boot order
    if (m_parser.isSet("bootorder")) {
        QString orderStr = m_parser.value("bootorder");
        QStringList orderParts = orderStr.split(',');
        QList<quint16> newOrder;

        for (const QString &part : orderParts) {
            bool ok;
            quint16 id = part.toUShort(&ok, 16);
            if (!ok) {
                err << "Error: Invalid bootorder format" << Qt::endl;
                return false;
            }
            newOrder.append(id);
        }

        list->setBootOrder(newOrder);
        if (!quiet) {
            out << "BootOrder set to: " << orderStr << Qt::endl;
        }
        modified = true;
    }

    // Handle delete bootorder
    if (m_parser.isSet("delete-bootorder")) {
        QList<quint16> emptyOrder;
        list->setBootOrder(emptyOrder);
        if (!quiet) {
            out << "BootOrder deleted" << Qt::endl;
        }
        modified = true;
    }

    // Handle BootNext
    if (m_parser.isSet("bootnext")) {
        QString bootnextStr = m_parser.value("bootnext");
        bool ok;
        quint16 bootnext = bootnextStr.toUShort(&ok, 16);
        if (!ok) {
            err << "Error: Invalid bootnext format" << Qt::endl;
            return false;
        }

        list->setBootNext(bootnext);
        if (!quiet) {
            out << "BootNext set to: " << QString::number(bootnext, 16).rightJustified(4, '0').toUpper() << Qt::endl;
        }
        modified = true;
    }

    // Handle delete BootNext
    if (m_parser.isSet("delete-bootnext")) {
        qefi_set_variable(g_efiUuid, QStringLiteral("BootNext"), QByteArray());
        if (!quiet) {
            out << "BootNext deleted" << Qt::endl;
        }
        modified = true;
    }

    // Handle timeout
    if (m_parser.isSet("timeout")) {
        QString timeoutStr = m_parser.value("timeout");
        bool ok;
        quint16 timeout = timeoutStr.toUShort(&ok);
        if (!ok) {
            err << "Error: Invalid timeout format" << Qt::endl;
            return false;
        }

        list->setTimeout(timeout);
        qefi_set_variable_uint16(g_efiUuid, QStringLiteral("Timeout"), timeout);
        if (!quiet) {
            out << "Timeout set to: " << timeout << " seconds" << Qt::endl;
        }
        modified = true;
    }

    // Handle delete timeout
    if (m_parser.isSet("delete-timeout")) {
        qefi_set_variable(g_efiUuid, QStringLiteral("Timeout"), QByteArray());
        if (!quiet) {
            out << "Timeout deleted" << Qt::endl;
        }
        modified = true;
    }

    // Handle remove duplicates
    if (m_parser.isSet("remove-dups")) {
        QList<quint16> order = list->order();
        QList<quint16> uniqueOrder;
        for (quint16 id : order) {
            if (!uniqueOrder.contains(id)) {
                uniqueOrder.append(id);
            }
        }
        if (uniqueOrder.size() != order.size()) {
            list->setBootOrder(uniqueOrder);
            if (!quiet) {
                out << "Removed duplicate entries from BootOrder" << Qt::endl;
            }
            modified = true;
        } else {
            if (!quiet) {
                out << "No duplicate entries found in BootOrder" << Qt::endl;
            }
        }
    }

    // Handle create boot entry
    if (m_parser.isSet("create") || m_parser.isSet("create-only")) {
        err << "Error: Boot entry creation is not yet implemented in CLI mode" << Qt::endl;
        err << "Please use the GUI mode for creating boot entries" << Qt::endl;
        return false;
    }

    return true; // Success (whether or not modifications were made)
}

int CLI::execute()
{
    m_parser.process(*m_app);

    // Check for version flag
    if (m_parser.isSet("version")) {
        printVersion();
        return 0;
    }

    // Check privileges
    if (!qefi_has_privilege()) {
        QTextStream err(stderr);
        err << "Error: Insufficient privileges. Please run as root or with elevated privileges." << Qt::endl;
        return 1;
    }

    // Test scan option might need privileges but not EFI
    if (m_parser.isSet("test-scan")) {
        return runPartitionScanTest();
    }

    // Check EFI availability and privileges
    if (!qefi_is_available()) {
        QTextStream err(stderr);
        err << "Error: No EFI environment available" << Qt::endl;
        return 1;
    }

    // Load boot entries
    QEFIEntryStaticList *list = QEFIEntryStaticList::instance();
    list->load();

    // Handle modifications first
    bool hasModifications = m_parser.isSet("active") || m_parser.isSet("inactive") || m_parser.isSet("delete-bootnum") || m_parser.isSet("bootorder")
        || m_parser.isSet("delete-bootorder") || m_parser.isSet("bootnext") || m_parser.isSet("delete-bootnext") || m_parser.isSet("timeout")
        || m_parser.isSet("delete-timeout") || m_parser.isSet("remove-dups") || m_parser.isSet("create") || m_parser.isSet("create-only");

    if (hasModifications) {
        if (!handleModifications()) {
            return 1;
        }
        // Reload to show updated state
        list->load();
    }

    // Print boot entries (default behavior or after modifications)
    bool verbose = m_parser.isSet("verbose");
    bool quiet = m_parser.isSet("quiet");
    printBootEntries(verbose, quiet);

    return 0;
}
