#include "mainwindow.h"

#include <QApplication>
#include <qefientrystaticlist.h>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QEFIEntryStaticList *list = QEFIEntryStaticList::instance();
    list->load();

    MainWindow w;
    w.show();
    return a.exec();
}
