#include "helpers.h"
#include "qefientrystaticlist.h"



#include <QDebug>
#include <QtEndian>

#include <qefi.h>

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
// Fix namsapce change of hex and dec
#define hex Qt::hex
#define dec Qt::dec
#endif

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

QEFIEntryStaticList::~QEFIEntryStaticList()
{
    for (const auto &option: std::as_const(m_loadOptions)) {
        if (option) delete (option);
    }
    m_loadOptions.clear();
}

void QEFIEntryStaticList::load()
{
    for (auto *option : std::as_const(m_loadOptions)) {
        delete option;
    }
    m_loadOptions.clear();
    m_cachedItem.clear();
    m_entries.clear();
    m_order.clear();

    quint16 current = qefi_get_variable_uint16(g_efiUuid,
                                               QStringLiteral("BootCurrent"));
    qDebug() << "BootCurrent: " << hex << current;

    quint16 timeout = qefi_get_variable_uint16(g_efiUuid,
                                               QStringLiteral("Timeout"));
    qDebug() << "Timeout: " << dec << timeout << " seconds";
    m_timeout = timeout;

    QByteArray data = qefi_get_variable(g_efiUuid,
                                        QStringLiteral("BootOrder"));
    quint16 *order_num = (quint16 *)data.data();
    qDebug() << "BootOrder: ";
    for (int i = 0; i < data.size() / 2; i++, order_num++) {
        qDebug()  << hex << qFromLittleEndian<quint16>(*order_num) << ", ";
        m_order.append(qFromLittleEndian<quint16>(*order_num));
    }
    qDebug() << dec;

    auto loadOne = [this](quint16 id) {
        if (m_entries.contains(id))
            return;
        QString name = QStringLiteral("Boot%1").arg(id, 4, 16, QLatin1Char('0'));
        QByteArray boot_data = qefi_get_variable(g_efiUuid, name);
        if (boot_data.isEmpty())
            return;
        QEFILoadOption *loadOption = new QEFILoadOption(boot_data);
        if (!loadOption->isValidated()) {
            delete loadOption;
            return;
        }
        m_cachedItem.insert(id, boot_data);
        m_loadOptions.insert(id, loadOption);
        m_entries.insert(id, QEFIEntry(id, loadOption));
        qDebug() << hex << id << " " << m_entries[id].name();
    };

    for (quint16 order_id : std::as_const(m_order))
        loadOne(order_id);

    // Also surface BootXXXX variables that are not currently in BootOrder,
    // matching rhboot/efibootmgr listing behavior (needed for --create-only).
    for (quint16 id = 0; id < 0x100; ++id)
        loadOne(id);
}

void QEFIEntryStaticList::setBootNext(const quint16 &next)
{
    // TODO: Maybe do validation
    qefi_set_variable_uint16(g_efiUuid,
                             QStringLiteral("BootNext"), next);
}

void QEFIEntryStaticList::setBootOrder(const QList<quint16> &newOrder)
{
    // TODO: Maybe do validation
    qDebug() << "New order is " << newOrder;

    QByteArray orderBuffer(newOrder.size() * 2, 0);
    quint16 *p = (quint16 *)orderBuffer.data();
    for (const auto &i: newOrder) {
        *p = qToLittleEndian<quint16>(i);
        p++;
    }
    qefi_set_variable(g_efiUuid,
                      QStringLiteral("BootOrder"), orderBuffer);

    // Sync the order in this class, we currently have confiance on the list
    m_order = newOrder;
}

bool QEFIEntryStaticList::setBootVisibility(
    const quint16 bootID, bool visible)
{
    auto bootDataIter = m_cachedItem.find(bootID);
    if (bootDataIter != m_cachedItem.end()) {
        QByteArray &bootData = *bootDataIter;
        if (bootData.size() < 4) return false;

        // Set the data
        quint32 attribute = (
            (bootData[3] << 24) | (bootData[2] << 16) |
            (bootData[1] << 8) | (bootData[0])
        );
        if (visible ^ (attribute & QEFI_LOAD_OPTION_ACTIVE)) {
            // Visibility is changed
            if (visible) attribute |= 0x00000001;
            else attribute &= 0xFFFFFFFE;

            QString name = QStringLiteral("Boot%1").arg(bootID, 4, 16, QLatin1Char('0'));
            bootData[3] = (attribute >> 24);
            bootData[2] = ((attribute >> 16) & 0xFF);
            bootData[1] = ((attribute >> 8) & 0xFF);
            bootData[0] = (attribute & 0xFF);
            qefi_set_variable(g_efiUuid,
                                    name, bootData);

            return true;
        }
    }
    return false;
}

QByteArray QEFIEntryStaticList::getRawData(const quint16 bootID)
{
    QByteArray res;
    auto bootDataIter = m_cachedItem.find(bootID);
    if (bootDataIter != m_cachedItem.end()) {
        return *bootDataIter;
    }
    return res;
}

bool QEFIEntryStaticList::updateBootEntry(const quint16 bootID, const QByteArray &data)
{
    QEFILoadOption *loadOption = new QEFILoadOption(data);
    if (!loadOption->isValidated()) {
        // Load Option is invalidated, clear it
        delete loadOption;
        return false;
    }
    m_cachedItem.insert(bootID, data);
    auto iter = m_loadOptions.find(bootID);
    if (iter != m_loadOptions.end()) {
        // Exists, delete the old one
        if (*iter) delete (*iter);
    }
    m_loadOptions.insert(bootID, loadOption);

    // Write the data
    QString name = QStringLiteral("Boot%1").arg(bootID, 4, 16, QLatin1Char('0'));
    qefi_set_variable(g_efiUuid,
                      name, data);

    // Update entry
    QEFIEntry entry(bootID, loadOption);
    m_entries.insert(bootID, entry);
    return true;
}
