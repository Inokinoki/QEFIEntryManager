#include "mainwindow.h"
#include "qefipartitionmanager.h"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QResource>
#include <QTranslator>
#include <QCommandLineParser>
#include <qefientrystaticlist.h>
#include <qefi.h>
#include <iostream>

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
    } catch (const std::exception& e) {
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
        const QEFIPartitionInfo& part = partitions[i];
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

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("QEFIEntryManager");
    // TODO: Move this to a generated header file during building
    a.setApplicationVersion("0.4.1");

    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("A cross-platform EFI Entry Manager base on Qt");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption testScanOption("test-scan",
        "Test EFI partition scanning (CLI mode for testing)");
    parser.addOption(testScanOption);

    parser.process(a);

    // If --test-scan is specified, run partition scan test and exit
    if (parser.isSet(testScanOption)) {
        return runPartitionScanTest();
    }

    // Normal GUI mode
    a.setDesktopFileName("qefientrymanager");
    a.setWindowIcon(QIcon("../cc.inoki.qefientrymanager.png"));

    QTranslator translator;
    if (translator.load(QLocale(), QStringLiteral("app"), QStringLiteral("_"), QStringLiteral(":/translations"), QStringLiteral(".qm")))
    {
        a.installTranslator(&translator);
    }
    else
    {
        qDebug() << "Failed to load translation";
    }

    if (!qefi_is_available() || !qefi_has_privilege())
    {
        QMessageBox::critical(nullptr, QObject::tr("Error"),
                              QObject::tr("Permission insufficient or no EFI environment"));
        return 1;
    }

    QEFIEntryStaticList *list = QEFIEntryStaticList::instance();
    list->load();

    MainWindow w;
    w.show();
    return a.exec();
}
