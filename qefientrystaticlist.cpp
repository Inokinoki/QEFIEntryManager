#include "qefientrystaticlist.h"

#include <QUuid>
#include <iostream>

#include <QDebug>

QByteArray qefi_get_variable(QUuid uuid, QString name);
quint16 qefi_get_variable_uint16(QUuid uuid, QString name);
void qefi_set_variable_uint16(QUuid uuid, QString name, quint16 value);
void qefi_set_variable(QUuid uuid, QString name, QByteArray value);

QEFIEntryStaticList::QEFIEntryStaticList()
{
}

QMap<quint16, QEFIEntry> QEFIEntryStaticList::entries() const
{
    return m_entries;
}

QList<quint16> QEFIEntryStaticList::order() const
{
    return m_order;
}

void QEFIEntryStaticList::setOrder(const QList<quint16> &order)
{
    m_order = order;
}

void QEFIEntryStaticList::setTimeout(const quint16 &timeout)
{
    m_timeout = timeout;
}

quint16 QEFIEntryStaticList::timeout() const
{
    return m_timeout;
}

QEFIEntryStaticList *QEFIEntryStaticList::instance()
{
    static QEFIEntryStaticList list;
    return &list;
}

#include <iostream>

void QEFIEntryStaticList::load()
{
    quint16 current = qefi_get_variable_uint16(QUuid("8be4df61-93ca-11d2-aa0d-00e098032b8c"),
                                               QStringLiteral("BootCurrent"));
    std::cerr << "BootCurrent: " << std::hex << current << std::endl;

    quint16 timeout = qefi_get_variable_uint16(QUuid("8be4df61-93ca-11d2-aa0d-00e098032b8c"),
                                               QStringLiteral("Timeout"));
    std::cerr << "Timeout: " << std::dec << timeout << " seconds" << std::endl;
    m_timeout = timeout;

    QByteArray data = qefi_get_variable(QUuid("8be4df61-93ca-11d2-aa0d-00e098032b8c"),
                                        QStringLiteral("BootOrder"));
    quint16 *order_num = (quint16 *)data.data();
    std::cerr << "BootOrder: ";
    m_order.clear();
    for (int i = 0; i < data.size() / 2; i++, order_num++) {
        std::cerr << std::hex << *order_num << ", ";
        m_order.append(*order_num);
    }
    std::cerr << std::dec << std::endl;

    order_num = (quint16 *)data.data();
    m_entries.clear();
    for (int i = 0; i < data.size() / 2; i++, order_num++) {
        QString name = QString::asprintf("Boot%04X", *order_num);
        QByteArray boot_data = qefi_get_variable(QUuid("8be4df61-93ca-11d2-aa0d-00e098032b8c"),
                                                 name);
        QString entry_name;
        if (boot_data.size() > 6) { // TODO: Explain the magic number
            quint16 *c = (quint16 *)boot_data.data();
            c = c + 6 / 2;
            for (int index = 6; index < boot_data.size(); index += 2, c++) {
                if (*c == 0) break;
                entry_name.append(*c);
            }
        }

        // Add entry
        QEFIEntry entry(*order_num, entry_name);
        m_entries.insert(*order_num, entry);

        // Cache
        m_cachedItem.insert(*order_num, boot_data);

        std::cerr << std::hex << *order_num << " " << entry_name.toStdString();
        std::cerr << std::endl;
    }
}

void QEFIEntryStaticList::setBootNext(const quint16 &next)
{
    // TODO: Maybe do validation
    qefi_set_variable_uint16(QUuid("8be4df61-93ca-11d2-aa0d-00e098032b8c"),
                             QStringLiteral("BootNext"), next);
}

void QEFIEntryStaticList::setBootOrder(const QList<quint16> &newOrder)
{
    // TODO: Maybe do validation
    qDebug() << "New order is " << newOrder;

    QByteArray orderBuffer(newOrder.size() * 2, 0);
    quint16 *p = (quint16 *)orderBuffer.data();
    for (int i = 0; i < newOrder.size(); i++, p++) {
        *p = newOrder[i];
    }
    qefi_set_variable(QUuid("8be4df61-93ca-11d2-aa0d-00e098032b8c"),
                      QStringLiteral("BootOrder"), orderBuffer);
    // TODO: Sync the order in this class
}
