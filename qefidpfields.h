#ifndef QEFIDPFIELDS_H
#define QEFIDPFIELDS_H

#include <QWidget>
#include <QVariant>
#include <QString>
#include <QList>
#include <QPair>
#include <qefi.h>

// ============================================================================
// Field Type System
// ============================================================================

// Field type enumeration
enum class QEFIFieldType {
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    String,
    Path,
    UUID,
    HexData,
    IPv4Address,
    IPv6Address,
    MACAddress,
    Port,
    URL,
    Enum,
    ByteArray,
    BoolEnum,
    PartitionSelector,  // Partition selection with browse button
    FilePath            // File path with browse button (same as Path but explicit)
};

// Field metadata structure
struct QEFIFieldMeta {
    QString name;           // Display name
    QString propertyName;   // Property accessor name (e.g., "function", "device")
    QEFIFieldType type;     // Field type
    QVariant minValue;      // Minimum value (for numeric types)
    QVariant maxValue;      // Maximum value (for numeric types)
    bool hexDisplay;        // Display as hex for numeric types
    QList<QPair<QString, QVariant>> enumValues;  // For enum types
    QString tooltip;        // Optional tooltip

    // Default constructor for use in containers
    QEFIFieldMeta()
        : name(""), propertyName(""), type(QEFIFieldType::String), hexDisplay(false)
    {
    }

    QEFIFieldMeta(const QString &n, const QString &prop, QEFIFieldType t)
        : name(n), propertyName(prop), type(t), hexDisplay(false)
    {
        // Set default ranges based on type
        switch (type) {
        case QEFIFieldType::UInt8:
            minValue = 0;
            maxValue = 0xFF;
            break;
        case QEFIFieldType::UInt16:
        case QEFIFieldType::Port:
            minValue = 0;
            maxValue = 0xFFFF;
            break;
        case QEFIFieldType::UInt32:
            minValue = 0;
            maxValue = 0xFFFFFFFF;
            break;
        case QEFIFieldType::UInt64:
            minValue = QVariant::fromValue(quint64(0));
            maxValue = QVariant::fromValue(quint64(0xFFFFFFFFFFFFFFFFULL));
            break;
        default:
            break;
        }
    }

    QEFIFieldMeta &asHex() {
        hexDisplay = true;
        return *this;
    }

    QEFIFieldMeta &range(QVariant min, QVariant max) {
        minValue = min;
        maxValue = max;
        return *this;
    }

    QEFIFieldMeta &withTooltip(const QString &tip) {
        tooltip = tip;
        return *this;
    }

    QEFIFieldMeta &withEnumValues(const QList<QPair<QString, QVariant>> &values) {
        type = QEFIFieldType::Enum;
        enumValues = values;
        return *this;
    }
};

// ============================================================================
// Macro Helpers for Defining Fields
// ============================================================================

#define FIELD_UINT8(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::UInt8)

#define FIELD_UINT8_HEX(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::UInt8).asHex()

#define FIELD_UINT16(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::UInt16)

#define FIELD_UINT16_HEX(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::UInt16).asHex()

#define FIELD_UINT32(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::UInt32)

#define FIELD_UINT32_HEX(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::UInt32).asHex()

#define FIELD_UINT64(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::UInt64)

#define FIELD_UINT64_HEX(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::UInt64).asHex()

#define FIELD_STRING(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::String)

#define FIELD_PATH(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::Path)

#define FIELD_UUID(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::UUID)

#define FIELD_HEXDATA(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::HexData)

#define FIELD_IPV4(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::IPv4Address)

#define FIELD_IPV6(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::IPv6Address)

#define FIELD_MAC(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::MACAddress)

#define FIELD_PORT(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::Port)

#define FIELD_URL(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::URL)

#define FIELD_ENUM(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::Enum)

#define FIELD_BYTEARRAY(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::ByteArray)

#define FIELD_PARTITION(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::PartitionSelector)

#define FIELD_FILEPATH(displayName, propName) \
    QEFIFieldMeta(displayName, propName, QEFIFieldType::FilePath)

// Macro to begin field definition for a device path type
#define BEGIN_DP_FIELDS(className) \
    inline QList<QEFIFieldMeta> get##className##Fields() { \
        QList<QEFIFieldMeta> fields;

// Macro to add a field
#define ADD_FIELD(field) \
        fields << field;

// Macro to end field definition
#define END_DP_FIELDS() \
        return fields; \
    }

// ============================================================================
// Widget Factory
// ============================================================================

class QEFIFieldWidgetFactory {
public:
    // Create a widget for a field
    static QWidget* createWidget(const QEFIFieldMeta &field, QWidget *parent = nullptr);

    // Set widget value from QVariant
    static void setWidgetValue(QWidget *widget, const QEFIFieldMeta &field, const QVariant &value);

    // Get widget value as QVariant
    static QVariant getWidgetValue(QWidget *widget, const QEFIFieldMeta &field);

    // Create a label for the field
    static QString getFieldLabel(const QEFIFieldMeta &field);
};

// ============================================================================
// Field Definitions for All Device Path Types
// ============================================================================

// Hardware Device Paths
BEGIN_DP_FIELDS(QEFIDevicePathHardwarePCI)
    ADD_FIELD(FIELD_UINT8_HEX("Function", "function"))
    ADD_FIELD(FIELD_UINT8_HEX("Device", "device"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathHardwarePCCard)
    ADD_FIELD(FIELD_UINT8_HEX("Function", "function"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathHardwareMMIO)
    ADD_FIELD(FIELD_UINT32_HEX("Memory Type", "memoryType"))
    ADD_FIELD(FIELD_UINT64_HEX("Starting Address", "startingAddress"))
    ADD_FIELD(FIELD_UINT64_HEX("Ending Address", "endingAddress"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathHardwareVendor)
    ADD_FIELD(FIELD_UUID("Vendor GUID", "vendorGuid"))
    ADD_FIELD(FIELD_HEXDATA("Vendor Data", "vendorData"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathHardwareController)
    ADD_FIELD(FIELD_UINT32_HEX("Controller Number", "controller"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathHardwareBMC)
    ADD_FIELD(FIELD_UINT8("Interface Type", "interfaceType"))
    ADD_FIELD(FIELD_UINT64_HEX("Base Address", "baseAddress"))
END_DP_FIELDS()

// ACPI Device Paths
BEGIN_DP_FIELDS(QEFIDevicePathACPIHID)
    ADD_FIELD(FIELD_UINT32_HEX("HID", "hid"))
    ADD_FIELD(FIELD_UINT32_HEX("UID", "uid"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathACPIHIDEX)
    ADD_FIELD(FIELD_UINT32_HEX("HID", "hid"))
    ADD_FIELD(FIELD_UINT32_HEX("UID", "uid"))
    ADD_FIELD(FIELD_UINT32_HEX("CID", "cid"))
    ADD_FIELD(FIELD_STRING("HID String", "hidString"))
    ADD_FIELD(FIELD_STRING("UID String", "uidString"))
    ADD_FIELD(FIELD_STRING("CID String", "cidString"))
END_DP_FIELDS()

// Message Device Paths
BEGIN_DP_FIELDS(QEFIDevicePathMessageATAPI)
    ADD_FIELD(FIELD_UINT8("Primary/Secondary", "primary"))
    ADD_FIELD(FIELD_UINT8("Master/Slave", "slave"))
    ADD_FIELD(FIELD_UINT16("LUN", "lun"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageSCSI)
    ADD_FIELD(FIELD_UINT16("Target ID", "target"))
    ADD_FIELD(FIELD_UINT16("LUN", "lun"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageUSB)
    ADD_FIELD(FIELD_UINT8("Parent Port", "parentPort"))
    ADD_FIELD(FIELD_UINT8("Interface", "usbInterface"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageI2O)
    ADD_FIELD(FIELD_UINT32("Target", "target"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageMACAddr)
    ADD_FIELD(FIELD_MAC("MAC Address", "macAddress"))
    ADD_FIELD(FIELD_UINT8("Interface Type", "interfaceType"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageIPv4Addr)
    ADD_FIELD(FIELD_IPV4("Local IP Address", "localIPv4Address"))
    ADD_FIELD(FIELD_IPV4("Remote IP Address", "remoteIPv4Address"))
    ADD_FIELD(FIELD_PORT("Local Port", "localPort"))
    ADD_FIELD(FIELD_PORT("Remote Port", "remotePort"))
    ADD_FIELD(FIELD_UINT16("Protocol", "protocol")
        .withEnumValues({
            {"TCP", 6},
            {"UDP", 17}
        }))
    ADD_FIELD(FIELD_UINT8("Static IP Address", "staticIPAddress")
        .withEnumValues({
            {"DHCP", 0},
            {"Static", 1}
        }))
    ADD_FIELD(FIELD_IPV4("Gateway", "gateway"))
    ADD_FIELD(FIELD_IPV4("Subnet Mask", "netmask"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageIPv6Addr)
    ADD_FIELD(FIELD_IPV6("Local IP Address", "localIPv6Address"))
    ADD_FIELD(FIELD_IPV6("Remote IP Address", "remoteIPv6Address"))
    ADD_FIELD(FIELD_PORT("Local Port", "localPort"))
    ADD_FIELD(FIELD_PORT("Remote Port", "remotePort"))
    ADD_FIELD(FIELD_UINT16("Protocol", "protocol")
        .withEnumValues({
            {"TCP", 6},
            {"UDP", 17}
        }))
    ADD_FIELD(FIELD_UINT8("IP Address Origin", "ipAddressOrigin"))
    ADD_FIELD(FIELD_UINT8("Prefix Length", "prefixLength"))
    ADD_FIELD(FIELD_UINT8("Gateway IPv6 Address", "gatewayIPv6Address"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageUART)
    ADD_FIELD(FIELD_UINT32("Reserved", "reserved"))
    ADD_FIELD(FIELD_UINT64("Baud Rate", "baudRate")
        .withEnumValues({
            {"9600", 9600},
            {"19200", 19200},
            {"38400", 38400},
            {"57600", 57600},
            {"115200", 115200}
        }))
    ADD_FIELD(FIELD_UINT8("Data Bits", "dataBits")
        .withEnumValues({
            {"5", 5},
            {"6", 6},
            {"7", 7},
            {"8", 8}
        }))
    ADD_FIELD(FIELD_UINT8("Parity", "parity")
        .withEnumValues({
            {"None", 0},
            {"Odd", 1},
            {"Even", 2}
        }))
    ADD_FIELD(FIELD_UINT8("Stop Bits", "stopBits")
        .withEnumValues({
            {"1", 1},
            {"1.5", 15},
            {"2", 2}
        }))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageUSBClass)
    ADD_FIELD(FIELD_UINT16_HEX("Vendor ID", "vendorId"))
    ADD_FIELD(FIELD_UINT16_HEX("Product ID", "productId"))
    ADD_FIELD(FIELD_UINT8_HEX("Device Class", "deviceClass"))
    ADD_FIELD(FIELD_UINT8_HEX("Device Subclass", "deviceSubclass"))
    ADD_FIELD(FIELD_UINT8_HEX("Device Protocol", "deviceProtocol"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageLUN)
    ADD_FIELD(FIELD_UINT8("LUN", "lun"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageSATA)
    ADD_FIELD(FIELD_UINT16("HBA Port", "hbaPort"))
    ADD_FIELD(FIELD_UINT16("Port Multiplier Port", "portMultiplierPort"))
    ADD_FIELD(FIELD_UINT16("LUN", "lun"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageISCSI)
    ADD_FIELD(FIELD_UINT16("Protocol", "protocol")
        .withEnumValues({
            {"TCP", 0}
        }))
    ADD_FIELD(FIELD_UINT16_HEX("Options", "options"))
    ADD_FIELD(FIELD_HEXDATA("LUN", "lun"))
    ADD_FIELD(FIELD_UINT16("Target Portal Group Tag", "tpgt"))
    ADD_FIELD(FIELD_STRING("Target Name", "targetName"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageVLAN)
    ADD_FIELD(FIELD_UINT16("VLAN ID", "vlanID"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageNVME)
    ADD_FIELD(FIELD_UINT32("Namespace ID", "namespaceID"))
    ADD_FIELD(FIELD_HEXDATA("IEEE EUI-64", "ieeeEui64"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageURI)
    ADD_FIELD(FIELD_URL("URI", "uri"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageUFS)
    ADD_FIELD(FIELD_UINT8("Target ID", "targetID"))
    ADD_FIELD(FIELD_UINT8("LUN", "lun"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageSD)
    ADD_FIELD(FIELD_UINT8("Slot Number", "slotNumber"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageBT)
    ADD_FIELD(FIELD_MAC("Bluetooth Address", "address"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageWiFi)
    ADD_FIELD(FIELD_STRING("SSID", "ssid"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageEMMC)
    ADD_FIELD(FIELD_UINT8("Slot Number", "slotNumber"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageBTLE)
    ADD_FIELD(FIELD_MAC("Bluetooth LE Address", "address"))
    ADD_FIELD(FIELD_UINT8("Address Type", "addressType")
        .withEnumValues({
            {"Public", 0},
            {"Random", 1}
        }))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMessageNVDIMM)
    ADD_FIELD(FIELD_UUID("NVDIMM UUID", "uuid"))
END_DP_FIELDS()

// Media Device Paths
BEGIN_DP_FIELDS(QEFIDevicePathMediaHD)
    ADD_FIELD(FIELD_PARTITION("Partition", "partition")
        .withTooltip("Click 'Browse Partitions...' to select from available partitions"))
    ADD_FIELD(FIELD_UINT32("Partition Number", "partitionNumber"))
    ADD_FIELD(FIELD_UINT64("Partition Start", "start"))
    ADD_FIELD(FIELD_UINT64("Partition Size", "size"))
    ADD_FIELD(FIELD_HEXDATA("Partition Signature", "signature"))
    ADD_FIELD(FIELD_ENUM("Partition Format", "format")
        .withEnumValues({
            {"GPT", QEFIDevicePathMediaHD::QEFIDevicePathMediaHDFormat::GPT},
            {"MBR (Legacy)", QEFIDevicePathMediaHD::QEFIDevicePathMediaHDFormat::PCAT}
        }))
    ADD_FIELD(FIELD_ENUM("Signature Type", "signatureType")
        .withEnumValues({
            {"None", QEFIDevicePathMediaHD::QEFIDevicePathMediaHDSignatureType::NONE},
            {"MBR Signature", QEFIDevicePathMediaHD::QEFIDevicePathMediaHDSignatureType::MBR},
            {"GUID", QEFIDevicePathMediaHD::QEFIDevicePathMediaHDSignatureType::GUID}
        }))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMediaCDROM)
    ADD_FIELD(FIELD_UINT32("Boot Catalog Entry", "bootCatalogEntry"))
    ADD_FIELD(FIELD_UINT64("Partition Start (RBA)", "partitionRba"))
    ADD_FIELD(FIELD_UINT64("Partition Size (Sectors)", "sectors"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMediaFile)
    ADD_FIELD(FIELD_FILEPATH("File Path", "name")
        .withTooltip("Click 'Browse File...' to select a file"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMediaVendor)
    ADD_FIELD(FIELD_UUID("Vendor GUID", "vendorGuid"))
    ADD_FIELD(FIELD_HEXDATA("Vendor Data", "vendorData"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMediaProtocol)
    ADD_FIELD(FIELD_UUID("Protocol GUID", "protocolGuid"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMediaFirmwareFile)
    ADD_FIELD(FIELD_HEXDATA("PI FFS Section Information", "piInfo"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMediaFirmwareVolume)
    ADD_FIELD(FIELD_HEXDATA("PI Firmware Volume Information", "piInfo"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMediaRelativeOffset)
    ADD_FIELD(FIELD_UINT32("Reserved", "reserved"))
    ADD_FIELD(FIELD_UINT64("Starting Offset", "firstByte"))
    ADD_FIELD(FIELD_UINT64("Ending Offset", "lastByte"))
END_DP_FIELDS()

BEGIN_DP_FIELDS(QEFIDevicePathMediaRAMDisk)
    ADD_FIELD(FIELD_UINT64_HEX("Start Address", "startAddress"))
    ADD_FIELD(FIELD_UINT64_HEX("End Address", "endAddress"))
    ADD_FIELD(FIELD_UUID("Disk Type GUID", "diskTypeGuid"))
    ADD_FIELD(FIELD_UINT16("Instance Number", "instanceNumber"))
END_DP_FIELDS()

// BIOS Boot Device Path
BEGIN_DP_FIELDS(QEFIDevicePathBIOSBoot)
    ADD_FIELD(FIELD_UINT16("Device Type", "deviceType"))
    ADD_FIELD(FIELD_UINT16("Status Flags", "status"))
    ADD_FIELD(FIELD_STRING("Description", "description"))
END_DP_FIELDS()

// Helper function to get fields for any device path
QList<QEFIFieldMeta> getDevicePathFields(QEFIDevicePath *dp);

#endif // QEFIDPFIELDS_H
