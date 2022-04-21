#ifndef HELPERS_H
#define HELPERS_H

#include <qefi.h>

#define DISPLAY_DATA_LIMIT 16

QString convert_device_path_type_to_name(QEFIDevicePathType type);
QString convert_device_path_subtype_to_name(QEFIDevicePathType type, quint8 subtype);

QList<QPair<QString, QString>> convert_device_path_attrs(QEFIDevicePath *dp);

#endif // HELPERS_H