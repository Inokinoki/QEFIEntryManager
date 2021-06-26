#ifndef QEFIENTRY_H
#define QEFIENTRY_H

#include <QString>
#include <QByteArray>

class QEFIEntry
{
    quint16 m_id;
    QString m_name;

    // TODO: Parse more information
public:
    QEFIEntry();
    QEFIEntry(quint16 id, QString name);
    QEFIEntry(quint16 id, QByteArray byteArrayFromEFIVar);
    QString name() const;
    quint16 id() const;
};

#endif // QEFIENTRY_H
