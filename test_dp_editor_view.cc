#include <QMap>
#include <QSharedPointer>
#include <QVariant>
#include <QtEndian>
#include <QtTest/QtTest>

#include "../helpers.h"
#include "../qefivar/qefi.h"

/* EFI device path header */
#pragma pack(push, 1)
struct qefi_device_path_header {
    quint8 type;
    quint8 subtype;
    quint16 length;
};
#pragma pack(pop)

// Forward declarations for parsing/formatting
QEFIDevicePath *qefi_parse_dp(struct qefi_device_path_header *dp, int dp_size);
QByteArray qefi_format_dp(QEFIDevicePath *dp);
QByteArray qefi_rfc4122_to_guid(const QByteArray data);

// Helper function to extract values from a device path for comparison
QMap<QString, QVariant> extract_values_from_device_path(QEFIDevicePath *dp)
{
    QMap<QString, QVariant> values;
    if (dp == nullptr)
        return values;

    QEFIDevicePathType type = dp->type();
    quint8 subtype = dp->subType();

    switch (type) {
    case QEFIDevicePathType::DP_Hardware:
        switch (subtype) {
        case QEFIDevicePathHardwareSubType::HW_PCI:
            if (auto *p = dynamic_cast<QEFIDevicePathHardwarePCI *>(dp)) {
                values["Function"] = p->function();
                values["Device"] = p->device();
            }
            break;
        case QEFIDevicePathHardwareSubType::HW_PCCard:
            if (auto *p = dynamic_cast<QEFIDevicePathHardwarePCCard *>(dp)) {
                values["Function"] = p->function();
            }
            break;
        case QEFIDevicePathHardwareSubType::HW_MMIO:
            if (auto *p = dynamic_cast<QEFIDevicePathHardwareMMIO *>(dp)) {
                values["Memory Type"] = p->memoryType();
                values["Starting Address"] = p->startingAddress();
                values["Ending Address"] = p->endingAddress();
            }
            break;
        case QEFIDevicePathHardwareSubType::HW_Vendor:
            if (auto *p = dynamic_cast<QEFIDevicePathHardwareVendor *>(dp)) {
                values["GUID"] = p->vendorGuid();
                values["Data"] = p->vendorData();
            }
            break;
        case QEFIDevicePathHardwareSubType::HW_Controller:
            if (auto *p = dynamic_cast<QEFIDevicePathHardwareController *>(dp)) {
                values["Controller"] = p->controller();
            }
            break;
        case QEFIDevicePathHardwareSubType::HW_BMC:
            if (auto *p = dynamic_cast<QEFIDevicePathHardwareBMC *>(dp)) {
                values["Interface Type"] = p->interfaceType();
                values["Base Address"] = p->baseAddress();
            }
            break;
        }
        break;
    case QEFIDevicePathType::DP_ACPI:
        switch (subtype) {
        case QEFIDevicePathACPISubType::ACPI_HID:
            if (auto *p = dynamic_cast<QEFIDevicePathACPIHID *>(dp)) {
                values["HID"] = p->hid();
                values["UID"] = p->uid();
            }
            break;
        case QEFIDevicePathACPISubType::ACPI_HIDEX:
            if (auto *p = dynamic_cast<QEFIDevicePathACPIHIDEX *>(dp)) {
                values["HID"] = p->hid();
                values["UID"] = p->uid();
                values["CID"] = p->cid();
                values["HID String"] = p->hidString();
                values["UID String"] = p->uidString();
                values["CID String"] = p->cidString();
            }
            break;
        }
        break;
    case QEFIDevicePathType::DP_Message:
        switch (subtype) {
        case QEFIDevicePathMessageSubType::MSG_ATAPI:
            if (auto *p = dynamic_cast<QEFIDevicePathMessageATAPI *>(dp)) {
                values["Primary"] = p->primary();
                values["Slave"] = p->slave();
                values["Lun"] = p->lun();
            }
            break;
        case QEFIDevicePathMessageSubType::MSG_SCSI:
            if (auto *p = dynamic_cast<QEFIDevicePathMessageSCSI *>(dp)) {
                values["Target"] = p->target();
                values["Lun"] = p->lun();
            }
            break;
        case QEFIDevicePathMessageSubType::MSG_USB:
            if (auto *p = dynamic_cast<QEFIDevicePathMessageUSB *>(dp)) {
                values["Parent Port"] = p->parentPort();
                values["Interface"] = p->usbInterface();
            }
            break;
        case QEFIDevicePathMessageSubType::MSG_Vendor:
            if (auto *p = dynamic_cast<QEFIDevicePathMessageVendor *>(dp)) {
                values["GUID"] = p->vendorGuid();
                values["Data"] = p->vendorData();
            }
            break;
        case QEFIDevicePathMessageSubType::MSG_LUN:
            if (auto *p = dynamic_cast<QEFIDevicePathMessageLUN *>(dp)) {
                values["Lun"] = p->lun();
            }
            break;
        case QEFIDevicePathMessageSubType::MSG_SATA:
            if (auto *p = dynamic_cast<QEFIDevicePathMessageSATA *>(dp)) {
                values["HBA Port"] = p->hbaPort();
                values["Port Multiplier"] = p->portMultiplierPort();
                values["Lun"] = p->lun();
            }
            break;
        case QEFIDevicePathMessageSubType::MSG_VLAN:
            if (auto *p = dynamic_cast<QEFIDevicePathMessageVLAN *>(dp)) {
                values["VLAN ID"] = p->vlanID();
            }
            break;
        case QEFIDevicePathMessageSubType::MSG_URI:
            if (auto *p = dynamic_cast<QEFIDevicePathMessageURI *>(dp)) {
                values["URI"] = p->uri().toString();
            }
            break;
        case QEFIDevicePathMessageSubType::MSG_UFS:
            if (auto *p = dynamic_cast<QEFIDevicePathMessageUFS *>(dp)) {
                values["Target ID"] = p->targetID();
                values["Lun"] = p->lun();
            }
            break;
        case QEFIDevicePathMessageSubType::MSG_SD:
            if (auto *p = dynamic_cast<QEFIDevicePathMessageSD *>(dp)) {
                values["Slot"] = p->slotNumber();
            }
            break;
        case QEFIDevicePathMessageSubType::MSG_WiFi:
            if (auto *p = dynamic_cast<QEFIDevicePathMessageWiFi *>(dp)) {
                values["SSID"] = p->ssid();
            }
            break;
        case QEFIDevicePathMessageSubType::MSG_EMMC:
            if (auto *p = dynamic_cast<QEFIDevicePathMessageEMMC *>(dp)) {
                values["Slot"] = p->slotNumber();
            }
            break;
        case QEFIDevicePathMessageSubType::MSG_NVDIMM:
            if (auto *p = dynamic_cast<QEFIDevicePathMessageNVDIMM *>(dp)) {
                values["UUID"] = p->uuid();
            }
            break;
        }
        break;
    case QEFIDevicePathType::DP_Media:
        switch (subtype) {
        case QEFIDevicePathMediaSubType::MEDIA_HD:
            if (auto *p = dynamic_cast<QEFIDevicePathMediaHD *>(dp)) {
                values["Partition Num"] = p->partitionNumber();
                values["Start"] = p->start();
                values["Size"] = p->size();
                values["Format"] = p->format();
                values["Signature Type"] = p->signatureType();
                QByteArray sig((const char *)p->rawSignature(), 16);
                values["Signature"] = sig;
            }
            break;
        case QEFIDevicePathMediaSubType::MEDIA_CDROM:
            if (auto *p = dynamic_cast<QEFIDevicePathMediaCDROM *>(dp)) {
                values["Boot Catalogue Entry"] = p->bootCatalogEntry();
                values["Partition RBA"] = p->partitionRba();
                values["Sectors"] = p->sectors();
            }
            break;
        case QEFIDevicePathMediaSubType::MEDIA_Vendor:
            if (auto *p = dynamic_cast<QEFIDevicePathMediaVendor *>(dp)) {
                values["GUID"] = p->vendorGuid();
                values["Data"] = p->vendorData();
            }
            break;
        case QEFIDevicePathMediaSubType::MEDIA_File:
            if (auto *p = dynamic_cast<QEFIDevicePathMediaFile *>(dp)) {
                values["File"] = p->name();
            }
            break;
        case QEFIDevicePathMediaSubType::MEDIA_Protocol:
            if (auto *p = dynamic_cast<QEFIDevicePathMediaProtocol *>(dp)) {
                values["Protocol GUID"] = p->protocolGuid();
            }
            break;
        case QEFIDevicePathMediaSubType::MEDIA_FirmwareFile:
            if (auto *p = dynamic_cast<QEFIDevicePathMediaFirmwareFile *>(dp)) {
                values["PI Info"] = p->piInfo();
            }
            break;
        case QEFIDevicePathMediaSubType::MEDIA_RelativeOffset:
            if (auto *p = dynamic_cast<QEFIDevicePathMediaRelativeOffset *>(dp)) {
                values["First Byte"] = p->firstByte();
                values["Last Byte"] = p->lastByte();
            }
            break;
        case QEFIDevicePathMediaSubType::MEDIA_RamDisk:
            if (auto *p = dynamic_cast<QEFIDevicePathMediaRAMDisk *>(dp)) {
                values["Start Address"] = p->startAddress();
                values["End Address"] = p->endAddress();
                values["Disk Type GUID"] = p->diskTypeGuid();
                values["Instance Number"] = p->instanceNumber();
            }
            break;
        }
        break;
    case QEFIDevicePathType::DP_BIOSBoot:
        if (auto *p = dynamic_cast<QEFIDevicePathBIOSBoot *>(dp)) {
            values["Device Type"] = p->deviceType();
            values["Status"] = p->status();
            values["Description"] = p->description();
        }
        break;
    }
    return values;
}

// Helper to compare two device paths by formatting and parsing
bool compare_device_paths(QEFIDevicePath *dp1, QEFIDevicePath *dp2)
{
    if (dp1 == nullptr || dp2 == nullptr)
        return dp1 == dp2;

    // Format both to bytes
    QByteArray data1 = qefi_format_dp(dp1);
    QByteArray data2 = qefi_format_dp(dp2);

    if (data1.isEmpty() || data2.isEmpty())
        return false;

    // Parse both back
    struct qefi_device_path_header *header1 = (struct qefi_device_path_header *)data1.data();
    struct qefi_device_path_header *header2 = (struct qefi_device_path_header *)data2.data();

    QSharedPointer<QEFIDevicePath> parsed1(qefi_parse_dp(header1, data1.length()));
    QSharedPointer<QEFIDevicePath> parsed2(qefi_parse_dp(header2, data2.length()));

    if (parsed1.isNull() || parsed2.isNull())
        return false;

    // Compare by extracting values
    QMap<QString, QVariant> values1 = extract_values_from_device_path(parsed1.get());
    QMap<QString, QVariant> values2 = extract_values_from_device_path(parsed2.get());

    if (values1.size() != values2.size())
        return false;

    for (auto it = values1.begin(); it != values1.end(); ++it) {
        if (!values2.contains(it.key()) || values2[it.key()] != it.value()) {
            return false;
        }
    }

    return true;
}

class TestDPEditorView : public QObject
{
    Q_OBJECT
private slots:
    void test_create_dummy_device_path();
    void test_create_device_path_from_values_media_hd();
    void test_create_device_path_from_values_media_file();
    void test_create_device_path_from_values_media_cdrom();
    void test_create_device_path_from_values_media_vendor();
    void test_create_device_path_from_values_media_protocol();
    void test_create_device_path_from_values_media_relative_offset();
    void test_create_device_path_from_values_media_ramdisk();
    void test_create_device_path_from_values_hardware_pci();
    void test_create_device_path_from_values_hardware_vendor();
    void test_create_device_path_from_values_acpi_hid();
    void test_create_device_path_from_values_message_atapi();
    void test_create_device_path_from_values_message_scsi();
    void test_create_device_path_from_values_message_usb();
    void test_create_device_path_from_values_message_sata();
    void test_create_device_path_from_values_message_uri();
    void test_create_device_path_from_values_message_wifi();
    void test_round_trip_media_hd();
    void test_round_trip_media_file();
    void test_round_trip_hardware_pci();
    void test_round_trip_acpi_hid();
    void test_round_trip_message_usb();
};

void TestDPEditorView::test_create_dummy_device_path()
{
    // Test that dummy device paths can be created for all types
    QList<quint8> subtypes;

    // Hardware
    subtypes = enum_device_path_subtype(QEFIDevicePathType::DP_Hardware);
    for (auto subtype : subtypes) {
        QEFIDevicePath *dummy = create_dummy_device_path(QEFIDevicePathType::DP_Hardware, subtype);
        QVERIFY(dummy != nullptr);
        QVERIFY(dummy->type() == QEFIDevicePathType::DP_Hardware);
        QVERIFY(dummy->subType() == subtype);
        delete dummy;
    }

    // ACPI
    subtypes = enum_device_path_subtype(QEFIDevicePathType::DP_ACPI);
    for (auto subtype : subtypes) {
        QEFIDevicePath *dummy = create_dummy_device_path(QEFIDevicePathType::DP_ACPI, subtype);
        QVERIFY(dummy != nullptr);
        QVERIFY(dummy->type() == QEFIDevicePathType::DP_ACPI);
        QVERIFY(dummy->subType() == subtype);
        delete dummy;
    }

    // Message
    subtypes = enum_device_path_subtype(QEFIDevicePathType::DP_Message);
    for (auto subtype : subtypes) {
        QEFIDevicePath *dummy = create_dummy_device_path(QEFIDevicePathType::DP_Message, subtype);
        QVERIFY(dummy != nullptr);
        QVERIFY(dummy->type() == QEFIDevicePathType::DP_Message);
        QVERIFY(dummy->subType() == subtype);
        delete dummy;
    }

    // Media
    subtypes = enum_device_path_subtype(QEFIDevicePathType::DP_Media);
    for (auto subtype : subtypes) {
        QEFIDevicePath *dummy = create_dummy_device_path(QEFIDevicePathType::DP_Media, subtype);
        QVERIFY(dummy != nullptr);
        QVERIFY(dummy->type() == QEFIDevicePathType::DP_Media);
        QVERIFY(dummy->subType() == subtype);
        delete dummy;
    }

    // BIOSBoot
    subtypes = enum_device_path_subtype(QEFIDevicePathType::DP_BIOSBoot);
    for (auto subtype : subtypes) {
        QEFIDevicePath *dummy = create_dummy_device_path(QEFIDevicePathType::DP_BIOSBoot, subtype);
        QVERIFY(dummy != nullptr);
        QVERIFY(dummy->type() == QEFIDevicePathType::DP_BIOSBoot);
        QVERIFY(dummy->subType() == subtype);
        delete dummy;
    }
}

void TestDPEditorView::test_create_device_path_from_values_media_hd()
{
    QByteArray guid = qefi_rfc4122_to_guid(QUuid("df98065f-0102-4255-8d88-dfd07e3e1629").toRfc4122());
    QMap<QString, QVariant> values;
    values["Partition Num"] = 1;
    values["Start"] = (qulonglong)1024;
    values["Size"] = (qulonglong)(1024 * 1024 * 100);
    values["Format"] = QEFIDevicePathMediaHD::QEFIDevicePathMediaHDFormat::GPT;
    values["Signature Type"] = QEFIDevicePathMediaHD::QEFIDevicePathMediaHDSignatureType::GUID;
    values["Signature"] = guid;

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_Media, QEFIDevicePathMediaSubType::MEDIA_HD, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathMediaHD *dp = dynamic_cast<QEFIDevicePathMediaHD *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->partitionNumber() == 1);
    QVERIFY(dp->start() == 1024);
    QVERIFY(dp->size() == 1024 * 1024 * 100);
    QVERIFY(dp->format() == QEFIDevicePathMediaHD::QEFIDevicePathMediaHDFormat::GPT);
    QVERIFY(dp->signatureType() == QEFIDevicePathMediaHD::QEFIDevicePathMediaHDSignatureType::GUID);
    QVERIFY(dp->gptGuid() == QUuid("df98065f-0102-4255-8d88-dfd07e3e1629"));

    delete created;
}

void TestDPEditorView::test_create_device_path_from_values_media_file()
{
    QMap<QString, QVariant> values;
    values["File"] = QStringLiteral("\\EFI\\refind\\refind_x64.efi");

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_Media, QEFIDevicePathMediaSubType::MEDIA_File, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathMediaFile *dp = dynamic_cast<QEFIDevicePathMediaFile *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->name() == QStringLiteral("\\EFI\\refind\\refind_x64.efi"));

    delete created;
}

void TestDPEditorView::test_create_device_path_from_values_media_cdrom()
{
    QMap<QString, QVariant> values;
    values["Boot Catalogue Entry"] = 1;
    values["Partition RBA"] = (qulonglong)2048;
    values["Sectors"] = (qulonglong)1024;

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_Media, QEFIDevicePathMediaSubType::MEDIA_CDROM, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathMediaCDROM *dp = dynamic_cast<QEFIDevicePathMediaCDROM *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->bootCatalogEntry() == 1);
    QVERIFY(dp->partitionRba() == 2048);
    QVERIFY(dp->sectors() == 1024);

    delete created;
}

void TestDPEditorView::test_create_device_path_from_values_media_vendor()
{
    QMap<QString, QVariant> values;
    values["GUID"] = QUuid("12345678-1234-1234-1234-123456789abc");
    values["Data"] = QByteArray::fromHex("01020304");

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_Media, QEFIDevicePathMediaSubType::MEDIA_Vendor, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathMediaVendor *dp = dynamic_cast<QEFIDevicePathMediaVendor *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->vendorGuid() == QUuid("12345678-1234-1234-1234-123456789abc"));
    QVERIFY(dp->vendorData() == QByteArray::fromHex("01020304"));

    delete created;
}

void TestDPEditorView::test_create_device_path_from_values_media_protocol()
{
    QMap<QString, QVariant> values;
    values["Protocol GUID"] = QUuid("12345678-1234-1234-1234-123456789abc");

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_Media, QEFIDevicePathMediaSubType::MEDIA_Protocol, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathMediaProtocol *dp = dynamic_cast<QEFIDevicePathMediaProtocol *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->protocolGuid() == QUuid("12345678-1234-1234-1234-123456789abc"));

    delete created;
}

void TestDPEditorView::test_create_device_path_from_values_media_relative_offset()
{
    QMap<QString, QVariant> values;
    values["First Byte"] = (qulonglong)0;
    values["Last Byte"] = (qulonglong)1024;

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_Media, QEFIDevicePathMediaSubType::MEDIA_RelativeOffset, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathMediaRelativeOffset *dp = dynamic_cast<QEFIDevicePathMediaRelativeOffset *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->firstByte() == 0);
    QVERIFY(dp->lastByte() == 1024);

    delete created;
}

void TestDPEditorView::test_create_device_path_from_values_media_ramdisk()
{
    QMap<QString, QVariant> values;
    values["Start Address"] = (qulonglong)0x1000;
    values["End Address"] = (qulonglong)0x2000;
    values["Disk Type GUID"] = QUuid("12345678-1234-1234-1234-123456789abc");
    values["Instance Number"] = 1;

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_Media, QEFIDevicePathMediaSubType::MEDIA_RamDisk, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathMediaRAMDisk *dp = dynamic_cast<QEFIDevicePathMediaRAMDisk *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->startAddress() == 0x1000);
    QVERIFY(dp->endAddress() == 0x2000);
    QVERIFY(dp->diskTypeGuid() == QUuid("12345678-1234-1234-1234-123456789abc"));
    QVERIFY(dp->instanceNumber() == 1);

    delete created;
}

void TestDPEditorView::test_create_device_path_from_values_hardware_pci()
{
    QMap<QString, QVariant> values;
    values["Function"] = 0x55;
    values["Device"] = 0xAA;

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_Hardware, QEFIDevicePathHardwareSubType::HW_PCI, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathHardwarePCI *dp = dynamic_cast<QEFIDevicePathHardwarePCI *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->function() == 0x55);
    QVERIFY(dp->device() == 0xAA);

    delete created;
}

void TestDPEditorView::test_create_device_path_from_values_hardware_vendor()
{
    QMap<QString, QVariant> values;
    values["GUID"] = QUuid("12345678-1234-1234-1234-123456789abc");
    values["Data"] = QByteArray::fromHex("01020304");

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_Hardware, QEFIDevicePathHardwareSubType::HW_Vendor, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathHardwareVendor *dp = dynamic_cast<QEFIDevicePathHardwareVendor *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->vendorGuid() == QUuid("12345678-1234-1234-1234-123456789abc"));
    QVERIFY(dp->vendorData() == QByteArray::fromHex("01020304"));

    delete created;
}

void TestDPEditorView::test_create_device_path_from_values_acpi_hid()
{
    QMap<QString, QVariant> values;
    values["HID"] = 0x55555555;
    values["UID"] = 0xAAAAAAAA;

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_ACPI, QEFIDevicePathACPISubType::ACPI_HID, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathACPIHID *dp = dynamic_cast<QEFIDevicePathACPIHID *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->hid() == 0x55555555);
    QVERIFY(dp->uid() == 0xAAAAAAAA);

    delete created;
}

void TestDPEditorView::test_create_device_path_from_values_message_atapi()
{
    QMap<QString, QVariant> values;
    values["Primary"] = 1;
    values["Slave"] = 0;
    values["Lun"] = 0;

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_Message, QEFIDevicePathMessageSubType::MSG_ATAPI, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathMessageATAPI *dp = dynamic_cast<QEFIDevicePathMessageATAPI *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->primary() == 1);
    QVERIFY(dp->slave() == 0);
    QVERIFY(dp->lun() == 0);

    delete created;
}

void TestDPEditorView::test_create_device_path_from_values_message_scsi()
{
    QMap<QString, QVariant> values;
    values["Target"] = 5;
    values["Lun"] = 10;

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_Message, QEFIDevicePathMessageSubType::MSG_SCSI, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathMessageSCSI *dp = dynamic_cast<QEFIDevicePathMessageSCSI *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->target() == 5);
    QVERIFY(dp->lun() == 10);

    delete created;
}

void TestDPEditorView::test_create_device_path_from_values_message_usb()
{
    QMap<QString, QVariant> values;
    values["Parent Port"] = 2;
    values["Interface"] = 1;

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_Message, QEFIDevicePathMessageSubType::MSG_USB, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathMessageUSB *dp = dynamic_cast<QEFIDevicePathMessageUSB *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->parentPort() == 2);
    QVERIFY(dp->usbInterface() == 1);

    delete created;
}

void TestDPEditorView::test_create_device_path_from_values_message_sata()
{
    QMap<QString, QVariant> values;
    values["HBA Port"] = 0;
    values["Port Multiplier"] = 1;
    values["Lun"] = 0;

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_Message, QEFIDevicePathMessageSubType::MSG_SATA, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathMessageSATA *dp = dynamic_cast<QEFIDevicePathMessageSATA *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->hbaPort() == 0);
    QVERIFY(dp->portMultiplierPort() == 1);
    QVERIFY(dp->lun() == 0);

    delete created;
}

void TestDPEditorView::test_create_device_path_from_values_message_uri()
{
    QMap<QString, QVariant> values;
    values["URI"] = QStringLiteral("http://example.com/boot.efi");

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_Message, QEFIDevicePathMessageSubType::MSG_URI, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathMessageURI *dp = dynamic_cast<QEFIDevicePathMessageURI *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->uri().toString() == QStringLiteral("http://example.com/boot.efi"));

    delete created;
}

void TestDPEditorView::test_create_device_path_from_values_message_wifi()
{
    QMap<QString, QVariant> values;
    values["SSID"] = QStringLiteral("MyWiFiNetwork");

    QEFIDevicePath *created = create_device_path_from_values(QEFIDevicePathType::DP_Message, QEFIDevicePathMessageSubType::MSG_WiFi, values);
    QVERIFY(created != nullptr);

    QEFIDevicePathMessageWiFi *dp = dynamic_cast<QEFIDevicePathMessageWiFi *>(created);
    QVERIFY(dp != nullptr);
    QVERIFY(dp->ssid() == QStringLiteral("MyWiFiNetwork"));

    delete created;
}

void TestDPEditorView::test_round_trip_media_hd()
{
    // Create original device path
    QByteArray guid = qefi_rfc4122_to_guid(QUuid("df98065f-0102-4255-8d88-dfd07e3e1629").toRfc4122());
    QEFIDevicePathMediaHD original(1,
                                   1024,
                                   1024 * 1024 * 100,
                                   (quint8 *)guid.data(),
                                   QEFIDevicePathMediaHD::QEFIDevicePathMediaHDFormat::GPT,
                                   QEFIDevicePathMediaHD::QEFIDevicePathMediaHDSignatureType::GUID);

    // Extract values
    QMap<QString, QVariant> values = extract_values_from_device_path(&original);

    // Create new device path from values
    QEFIDevicePath *recreated = create_device_path_from_values(QEFIDevicePathType::DP_Media, QEFIDevicePathMediaSubType::MEDIA_HD, values);
    QVERIFY(recreated != nullptr);

    // Format both and compare
    QByteArray originalData = qefi_format_dp(&original);
    QByteArray recreatedData = qefi_format_dp(recreated);
    QVERIFY(!originalData.isEmpty());
    QVERIFY(!recreatedData.isEmpty());
    QVERIFY(originalData == recreatedData);

    delete recreated;
}

void TestDPEditorView::test_round_trip_media_file()
{
    QEFIDevicePathMediaFile original(QStringLiteral("\\EFI\\refind\\refind_x64.efi"));

    QMap<QString, QVariant> values = extract_values_from_device_path(&original);

    QEFIDevicePath *recreated = create_device_path_from_values(QEFIDevicePathType::DP_Media, QEFIDevicePathMediaSubType::MEDIA_File, values);
    QVERIFY(recreated != nullptr);

    QByteArray originalData = qefi_format_dp(&original);
    QByteArray recreatedData = qefi_format_dp(recreated);
    QVERIFY(!originalData.isEmpty());
    QVERIFY(!recreatedData.isEmpty());
    QVERIFY(originalData == recreatedData);

    delete recreated;
}

void TestDPEditorView::test_round_trip_hardware_pci()
{
    QEFIDevicePathHardwarePCI original(0x55, 0xAA);

    QMap<QString, QVariant> values = extract_values_from_device_path(&original);

    QEFIDevicePath *recreated = create_device_path_from_values(QEFIDevicePathType::DP_Hardware, QEFIDevicePathHardwareSubType::HW_PCI, values);
    QVERIFY(recreated != nullptr);

    QByteArray originalData = qefi_format_dp(&original);
    QByteArray recreatedData = qefi_format_dp(recreated);
    QVERIFY(!originalData.isEmpty());
    QVERIFY(!recreatedData.isEmpty());
    QVERIFY(originalData == recreatedData);

    delete recreated;
}

void TestDPEditorView::test_round_trip_acpi_hid()
{
    QEFIDevicePathACPIHID original(0x55555555, 0xAAAAAAAA);

    QMap<QString, QVariant> values = extract_values_from_device_path(&original);

    QEFIDevicePath *recreated = create_device_path_from_values(QEFIDevicePathType::DP_ACPI, QEFIDevicePathACPISubType::ACPI_HID, values);
    QVERIFY(recreated != nullptr);

    QByteArray originalData = qefi_format_dp(&original);
    QByteArray recreatedData = qefi_format_dp(recreated);
    QVERIFY(!originalData.isEmpty());
    QVERIFY(!recreatedData.isEmpty());
    QVERIFY(originalData == recreatedData);

    delete recreated;
}

void TestDPEditorView::test_round_trip_message_usb()
{
    QEFIDevicePathMessageUSB original(2, 1);

    QMap<QString, QVariant> values = extract_values_from_device_path(&original);

    QEFIDevicePath *recreated = create_device_path_from_values(QEFIDevicePathType::DP_Message, QEFIDevicePathMessageSubType::MSG_USB, values);
    QVERIFY(recreated != nullptr);

    QByteArray originalData = qefi_format_dp(&original);
    QByteArray recreatedData = qefi_format_dp(recreated);
    QVERIFY(!originalData.isEmpty());
    QVERIFY(!recreatedData.isEmpty());
    QVERIFY(originalData == recreatedData);

    delete recreated;
}

QTEST_MAIN(TestDPEditorView)
#include "test_dp_editor_view.moc"
