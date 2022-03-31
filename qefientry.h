#ifndef QEFIENTRY_H
#define QEFIENTRY_H

#include <QString>
#include <QByteArray>

class QEFIEntry
{
    quint16 m_id;
    QString m_name;
    QString m_devicePath;
    bool m_isActive;

    // TODO: Parse more information
public:
    QEFIEntry();
    QEFIEntry(quint16 id, QString name, QString m_devicePath);
    QEFIEntry(quint16 id, QByteArray byteArrayFromEFIVar);
    QString name() const;
    quint16 id() const;
    QString devicePath() const;
    bool isActive() const;
};

#endif // QEFIENTRY_H
