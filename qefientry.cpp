#include "qefientry.h"

QString QEFIEntry::name() const
{
    return m_name;
}

quint16 QEFIEntry::id() const
{
    return m_id;
}

QEFIEntry::QEFIEntry(quint16 id, QString name)
{
    m_id = id; m_name = name;
}

QEFIEntry::QEFIEntry(quint16 id, QByteArray boot_data)
{
    m_id = id;

    QString entry_name;
    if (boot_data.size() > 6) { // TODO: Explain the magic number
        quint16 *c = (quint16 *)boot_data.data();
        c = c + 6 / 2;
        for (int index = 6; index < boot_data.size(); index += 2, c++) {
            if (*c == 0) break;
            entry_name.append(*c);
        }
    }
    m_name = entry_name;
}
