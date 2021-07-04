#include "qefientry.h"

#include <QDebug>

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

QEFIEntry::QEFIEntry(quint16 id, QString name, QString devicePath)
{
    m_id = id; m_name = name; m_devicePath = devicePath;
}

QEFIEntry::QEFIEntry(quint16 id, QByteArray boot_data)
{
    m_id = id;
    m_name = qefi_extract_name(boot_data);
    m_devicePath = qefi_extract_path(boot_data);
}

QEFIEntry::QEFIEntry() {}
