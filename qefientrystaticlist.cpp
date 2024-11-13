#include "qefientrystaticlist.h"



#include <QDebug>
#include <QtEndian>

#include <qefi.h>

#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
// Fix namsapce change of hex and dec
#define hex Qt::hex
#define dec Qt::dec
#endif

// Defined in UEFI Spec as "EFI_GLOBAL_VARIABLE"
constexpr QUuid g_efiUuid = QUuid(0x8be4df61, 0x93ca, 0x11d2, 0xaa, 0x0d, 0x00,
                                  0xe0, 0x98, 0x03, 0x2b, 0x8c);

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
    m_order.clear();
    for (int i = 0; i < data.size() / 2; i++, order_num++) {
        qDebug()  << hex << qFromLittleEndian<quint16>(*order_num) << ", ";
        m_order.append(qFromLittleEndian<quint16>(*order_num));
    }
    qDebug() << dec;

    order_num = (quint16 *)data.data();
    m_entries.clear();
    for (int i = 0; i < data.size() / 2; i++, order_num++) {
        quint16 order_id = qFromLittleEndian<quint16>(*order_num);
        QString name = QStringLiteral("Boot%1").arg(order_id, 4, 16, QLatin1Char('0'));
        QByteArray boot_data = qefi_get_variable(g_efiUuid,
                                                 name);
        QEFILoadOption *loadOption = new QEFILoadOption(boot_data);

        // Cache
        m_cachedItem.insert(order_id, boot_data);
        m_loadOptions.insert(order_id, loadOption);

        // Add entry
        QEFIEntry entry(order_id, m_loadOptions[order_id]);
        m_entries.insert(order_id, entry);

        qDebug() << hex << order_id << " " << entry.name();
    }
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
    int index = m_order.indexOf(bootID);
    if (index == -1) return false;

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
    int index = m_order.indexOf(bootID);
    if (index == -1) return res;

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
