#ifndef HELPERS_H
#define HELPERS_H

#include <qefi.h>

#define DISPLAY_DATA_LIMIT 16

// Defined in UEFI Spec as "EFI_GLOBAL_VARIABLE"
constexpr QUuid g_efiUuid(0x8be4df61, 0x93ca, 0x11d2, 0xaa, 0x0d,
                          0x00, 0xe0, 0x98, 0x03, 0x2b, 0x8c);

constexpr QUuid g_efiPartTypeGuid(0xc12a7328, 0xf81f, 0x11d2, 0xba, 0x4b,
                                  0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b);

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

// Create a dummy device path instance for a given type/subtype
// Used to get field definitions when creating new device paths
QEFIDevicePath *create_dummy_device_path(QEFIDevicePathType type, quint8 subtype);

// Create a device path from UI values
QEFIDevicePath *create_device_path_from_values(QEFIDevicePathType type, quint8 subtype, const QMap<QString, QVariant> &values);

#endif // HELPERS_H
