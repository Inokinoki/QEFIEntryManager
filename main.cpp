#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>
#include <qefientrystaticlist.h>
#include <qefi.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setDesktopFileName("qefientrymanager");
    a.setWindowIcon(QIcon("../cc.inoki.qefientrymanager.png"));

    if (!qefi_is_available() || !qefi_has_privilege()) {
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
