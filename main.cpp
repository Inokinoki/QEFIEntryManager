#include "mainwindow.h"

#include <QApplication>

QByteArray qefi_get_variable(QUuid uuid, QString name);
quint16 qefi_get_variable_uint16(QUuid uuid, QString name);

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
