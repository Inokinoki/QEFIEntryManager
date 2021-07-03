#include "qefientry.h"

#include <QDebug>

extern "C" {
#include <efivar/efivar.h>
#include <efivar/efiboot-loadopt.h>
#include <efivar/efivar-dp.h>
}

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

    // TODO: Move these code to qefi, add support for Windows
    char text_path[4096];
	size_t text_path_len = 4096;
	uint16_t pathlen;
	ssize_t rc;
	efidp dp = NULL;

    efi_load_option *load_option = (efi_load_option *)boot_data.data();
	pathlen = efi_loadopt_pathlen(load_option, boot_data.size());
	dp = efi_loadopt_path(load_option, boot_data.size());
	rc = efidp_format_device_path(text_path, text_path_len, dp, pathlen);
    qDebug() << "[" << rc << "]" << text_path;
    if (rc > 0) {
        m_devicePath = QString(text_path);
    }
}

QEFIEntry::QEFIEntry() {}
