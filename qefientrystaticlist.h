#ifndef QEFIENTRYSTATICLIST_H
#define QEFIENTRYSTATICLIST_H

#include <QList>
#include <QMap>
#include <QUuid>

#include <qefientry.h>

class QEFIEntryStaticList
{
private:
    QEFIEntryStaticList();

    QMap<quint16, QEFIEntry> m_entries;     // Entries
    QMap<quint16, QByteArray> m_cachedItem; // Cached EFI array
    QList<quint16> m_order;                 // Cached order
    quint16 m_timeout;

    QMap<quint16, QEFILoadOption *> m_loadOptions;    // Cached LoadOption array

public:
    static QEFIEntryStaticList *instance();
    void load();    // TODO: Add an async implementation for progress display

    quint16 timeout() const;
    void setTimeout(const quint16 &timeout);
    QList<quint16> order() const;
    void setOrder(const QList<quint16> &order);
    QMap<quint16, QEFIEntry> entries() const;

    void setBootNext(const quint16 &next);
    void setBootOrder(const QList<quint16> &newOrder);
    bool setBootVisibility(const quint16 bootID, bool visible);

    QByteArray getRawData(const quint16 bootID);
    bool updateBootEntry(const quint16 bootID, const QByteArray &data);

    virtual ~QEFIEntryStaticList();
};

#endif // QEFIENTRYSTATICLIST_H
