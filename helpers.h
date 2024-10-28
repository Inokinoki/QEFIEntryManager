#ifndef HELPERS_H
#define HELPERS_H

#include <qefi.h>

#define DISPLAY_DATA_LIMIT 16

QString convert_device_path_type_to_name(QEFIDevicePathType type);
QString convert_device_path_subtype_to_name(QEFIDevicePathType type, quint8 subtype);

QList<QPair<QString, QString>> convert_device_path_attrs(QEFIDevicePath *dp);

enum QEFIDPEditType {
    EditType_Text,
    EditType_Path,
    EditType_Number,
    EditType_HexNumber,
    EditType_Enum,
    EditType_HexData,
    EditType_UUID
};
QList<QPair<QString, enum QEFIDPEditType>> convert_device_path_types(QEFIDevicePath *dp);
QString convert_ipv4_to_string(const QEFIIPv4Address *ipv4);
QList<quint8> enum_device_path_subtype(QEFIDevicePathType type);

#endif // HELPERS_H
