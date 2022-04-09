#ifndef QEFIENTRY_H
#define QEFIENTRY_H

#include <QString>
#include <QByteArray>

#include <qefi.h>

class QEFIEntry
{
    QEFILoadOption *m_loadOption;   // A weak reference

    quint16 m_id;
    QString m_name;
    QString m_devicePath;
    bool m_isActive;

    // TODO: Parse more information
public:
    QEFIEntry();
    QEFIEntry(quint16 id, QString name, QString m_devicePath);
    QEFIEntry(quint16 id, QByteArray byteArrayFromEFIVar);
    QEFIEntry(quint16 id, QEFILoadOption *loadOption);
    QString name() const;
    quint16 id() const;
    QString devicePath() const;
    bool isActive() const;
    QEFILoadOption *loadOption() const;
};

#endif // QEFIENTRY_H
