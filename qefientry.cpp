#include "qefientry.h"

#include <QDebug>
#include <QtEndian>

#include <qefi.h>

QString QEFIEntry::name() const
{
    return m_name;
}

quint16 QEFIEntry::id() const
{
    return m_id;
}

QString QEFIEntry::devicePath() const
{
    return m_devicePath;
}

bool QEFIEntry::isActive() const
{
    return m_isActive;
}

QEFIEntry::QEFIEntry(quint16 id, QString name, QString devicePath)
{
    m_id = id; m_name = name; m_devicePath = devicePath; m_isActive = true;
}

QEFIEntry::QEFIEntry(quint16 id, QByteArray boot_data)
{
    m_id = id;
    m_name = qefi_extract_name(boot_data);
    m_devicePath = qefi_extract_path(boot_data);
    m_isActive = (qFromLittleEndian<quint32>(*((quint32 *)boot_data.data())) & 0x00000001);
}

QEFIEntry::QEFIEntry() {}
