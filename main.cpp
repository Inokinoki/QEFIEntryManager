#include "mainwindow.h"
#include "version.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QMessageBox>
#include <QResource>
#include <QTranslator>
#include <qefi.h>
#include <qefientrystaticlist.h>

int main(int argc, char *argv[])
{
    // GUI mode if no CLI mode is specified
    QApplication a(argc, argv);
    a.setApplicationName("QEFIEntryManager");
    a.setApplicationVersion(QEFI_ENTRY_MANAGER_VERSION);

    // Normal GUI mode
    a.setDesktopFileName("qefientrymanager");
    a.setWindowIcon(QIcon("../cc.inoki.qefientrymanager.png"));

    QTranslator translator;
    if (translator.load(QLocale(), QStringLiteral("app"), QStringLiteral("_"), QStringLiteral(":/translations"), QStringLiteral(".qm"))) {
        a.installTranslator(&translator);
    } else {
        qDebug() << "Failed to load translation";
    }

    if (!qefi_is_available() || !qefi_has_privilege()) {
        QMessageBox::critical(nullptr, QObject::tr("Error"), QObject::tr("Permission insufficient or no EFI environment"));
        return 1;
    }

    QEFIEntryStaticList *list = QEFIEntryStaticList::instance();
    list->load();

    MainWindow w;
    w.show();
    return a.exec();
}
