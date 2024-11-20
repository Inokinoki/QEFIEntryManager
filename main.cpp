#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QResource>
#include <QTranslator>
#include <qefientrystaticlist.h>
#include <qefi.h>

int main(int argc, char *argv[])
{
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
