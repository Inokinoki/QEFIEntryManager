#include "mainwindow.h"
#include "cli.h"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QResource>
#include <QTranslator>
#include <qefientrystaticlist.h>
#include <qefi.h>

// Check if we should run in CLI mode
bool shouldUseCLIMode(int argc, char *argv[])
{
    // If there are command-line arguments (other than program name), use CLI mode
    for (int i = 1; i < argc; i++) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        // Any argument triggers CLI mode
        if (!arg.isEmpty()) {
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[])
{
    // Detect CLI mode early
    if (shouldUseCLIMode(argc, argv)) {
        // CLI mode
        CLI cli(argc, argv);
        return cli.execute();
    }

    // GUI mode (original behavior)
    QApplication a(argc, argv);
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
