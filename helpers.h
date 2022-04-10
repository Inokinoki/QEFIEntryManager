#ifndef HELPERS_H
#define HELPERS_H

#include <qefi.h>

QString convert_device_path_type_to_name(QEFIDevicePathType type);
QString convert_device_path_subtype_to_name(QEFIDevicePathType type, quint8 subtype);

#endif // HELPERS_H