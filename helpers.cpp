#include "helpers.h"

#include <QHostAddress>

// Helpers to convert device path to string
QString convert_device_path_subtype_to_name(
    QEFIDevicePathType type, quint8 subtype)
{
    switch (type)
    {
    case QEFIDevicePathType::DP_Hardware:
        switch (subtype) {
            case QEFIDevicePathHardwareSubType::HW_PCI:
                return "PCI";
            case QEFIDevicePathHardwareSubType::HW_PCCard:
                return "PCCard";
            case QEFIDevicePathHardwareSubType::HW_MMIO:
                return "MMIO";
            case QEFIDevicePathHardwareSubType::HW_Vendor:
                return "Vendor";
            case QEFIDevicePathHardwareSubType::HW_Controller:
                return "Controller";
            case QEFIDevicePathHardwareSubType::HW_BMC:
                return "BMC";
        }
        break;
    case QEFIDevicePathType::DP_ACPI:
        switch (subtype) {
            case QEFIDevicePathACPISubType::ACPI_HID:
                return "HID";
            case QEFIDevicePathACPISubType::ACPI_HIDEX:
                return "HIDEX";
            case QEFIDevicePathACPISubType::ACPI_ADR:
                return "ADR";
        }
        break;
    case QEFIDevicePathType::DP_Message:
        switch (subtype) {
            case QEFIDevicePathMessageSubType::MSG_ATAPI:
                return "ATAPI";
            case QEFIDevicePathMessageSubType::MSG_SCSI:
                return "SCSI";
            case QEFIDevicePathMessageSubType::MSG_FibreChan:
                return "Fibre Channel";
            case QEFIDevicePathMessageSubType::MSG_1394:
                return "1394";
            case QEFIDevicePathMessageSubType::MSG_USB:
                return "USB";
            case QEFIDevicePathMessageSubType::MSG_I2O:
                return "I2O";
            case QEFIDevicePathMessageSubType::MSG_InfiniBand:
                return "Inifiband";
            case QEFIDevicePathMessageSubType::MSG_Vendor:
                return "Vendor";
            case QEFIDevicePathMessageSubType::MSG_MACAddr:
                return "MAC";
            case QEFIDevicePathMessageSubType::MSG_IPv4:
                return "IPv4";
            case QEFIDevicePathMessageSubType::MSG_IPv6:
                return "IPv6";
            case QEFIDevicePathMessageSubType::MSG_UART:
                return "UART";
            case QEFIDevicePathMessageSubType::MSG_USBClass:
                return "USB Class";

            case QEFIDevicePathMessageSubType::MSG_USBWWID:
                return "USB WWID";

            case QEFIDevicePathMessageSubType::MSG_LUN:
                return "LUN";
            case QEFIDevicePathMessageSubType::MSG_SATA:
                return "SATA";
            case QEFIDevicePathMessageSubType::MSG_ISCSI:
                return "ISCSI";
            case QEFIDevicePathMessageSubType::MSG_VLAN:
                return "VLAN";

            case QEFIDevicePathMessageSubType::MSG_FibreChanEx:
                return "Fibre Channel Ex";
            case QEFIDevicePathMessageSubType::MSG_SASEX:
                return "SAS Ex";

            case QEFIDevicePathMessageSubType::MSG_NVME:
                return "NVME";
            case QEFIDevicePathMessageSubType::MSG_URI:
                return "URI";
            case QEFIDevicePathMessageSubType::MSG_UFS:
                return "UFS";
            case QEFIDevicePathMessageSubType::MSG_SD:
                return "SD";
            case QEFIDevicePathMessageSubType::MSG_BT:
                return "Bluetooth";
            case QEFIDevicePathMessageSubType::MSG_WiFi:
                return "WiFi";
            case QEFIDevicePathMessageSubType::MSG_EMMC:
                return "EMMC";
            case QEFIDevicePathMessageSubType::MSG_BTLE:
                return "BLE";
            case QEFIDevicePathMessageSubType::MSG_DNS:
                return "DNS";
            case QEFIDevicePathMessageSubType::MSG_NVDIMM:
                return "NVDIMM";
        }
        break;
    case QEFIDevicePathType::DP_Media:
        switch (subtype) {
            case QEFIDevicePathMediaSubType::MEDIA_HD:
                return "HD";
            case QEFIDevicePathMediaSubType::MEDIA_File:
                return "File";
            case QEFIDevicePathMediaSubType::MEDIA_CDROM:
                return "CDROM";
            case QEFIDevicePathMediaSubType::MEDIA_Vendor:
                return "Vendor";
            case QEFIDevicePathMediaSubType::MEDIA_Protocol:
                return "Protocol";
            case QEFIDevicePathMediaSubType::MEDIA_FirmwareFile:
                return "Firmware file";
            case QEFIDevicePathMediaSubType::MEDIA_FirmwareVolume:
                return "FV";
            case QEFIDevicePathMediaSubType::MEDIA_RelativeOffset:
                return "Relative offset";
            case QEFIDevicePathMediaSubType::MEDIA_RamDisk:
                return "RAMDisk";
        }
    case QEFIDevicePathType::DP_BIOSBoot:
        return "Boot";
    }
    return "Unknown";
}

QString convert_device_path_type_to_name(QEFIDevicePathType type)
{
    switch (type)
    {
    case DP_Hardware:
        return "Hardware";
    case DP_ACPI:
        return "ACPI";
    case DP_Message:
        return "Message";
    case DP_Media:
        return "Media";
    case DP_BIOSBoot:
        return "BIOS";
    }
    return "Unknown";
}

QList<QPair<QString, QString>> convert_device_path_attrs(QEFIDevicePath *dp)
{
    if (dp == nullptr) return {};
    QList<QPair<QString, QString>> list;
    QEFIDevicePathType type = dp->type();
    quint8 subtype = dp->subType();
    // TODO: Upgrade qefivar and parse more device paths
    switch (type)
    {
    case QEFIDevicePathType::DP_Hardware:
        switch (subtype) {
            case QEFIDevicePathHardwareSubType::HW_PCI:
                if (dynamic_cast<QEFIDevicePathHardwarePCI *>(dp)) {
                    QEFIDevicePathHardwarePCI *dpPCI = (QEFIDevicePathHardwarePCI *)
                        dynamic_cast<QEFIDevicePathHardwarePCI *>(dp);
                    list << qMakePair<QString, QString>("Function",
                        QString::number(dpPCI->function()));
                    list << qMakePair<QString, QString>("Device",
                        QString::number(dpPCI->device()));
                }
                break;
            case QEFIDevicePathHardwareSubType::HW_PCCard:
                if (dynamic_cast<QEFIDevicePathHardwarePCCard *>(dp)) {
                    QEFIDevicePathHardwarePCCard *dpPCCard = (QEFIDevicePathHardwarePCCard *)
                        dynamic_cast<QEFIDevicePathHardwarePCCard *>(dp);
                    list << qMakePair<QString, QString>("Function",
                        QString::number(dpPCCard->function()));
                }
                break;
            case QEFIDevicePathHardwareSubType::HW_MMIO:
                if (dynamic_cast<QEFIDevicePathHardwareMMIO *>(dp)) {
                    QEFIDevicePathHardwareMMIO *dpMMIO = (QEFIDevicePathHardwareMMIO *)
                        dynamic_cast<QEFIDevicePathHardwareMMIO *>(dp);
                    list << qMakePair<QString, QString>("Memory Type",
                        QString::number(dpMMIO->memoryType()));
                    list << qMakePair<QString, QString>("Starting Address",
                        QString::number(dpMMIO->startingAddress()));
                    list << qMakePair<QString, QString>("Ending Address",
                        QString::number(dpMMIO->endingAddress()));
                }
                break;
            case QEFIDevicePathHardwareSubType::HW_Vendor:
                if (dynamic_cast<QEFIDevicePathHardwareVendor *>(dp)) {
                    QEFIDevicePathHardwareVendor *dpVendor = (QEFIDevicePathHardwareVendor *)
                        dynamic_cast<QEFIDevicePathHardwareVendor *>(dp);
                    list << qMakePair<QString, QString>("GUID",
                        dpVendor->vendorGuid().toString());
                    QByteArray data = dpVendor->vendorData().toHex();
                    if (data.size() > DISPLAY_DATA_LIMIT * 2) {
                        data.truncate(DISPLAY_DATA_LIMIT * 2);
                        data += "...";
                    }
                    list << qMakePair<QString, QString>("Data", data);
                }
                break;
            case QEFIDevicePathHardwareSubType::HW_Controller:
                if (dynamic_cast<QEFIDevicePathHardwareController *>(dp)) {
                    QEFIDevicePathHardwareController *dpController =
                        (QEFIDevicePathHardwareController *)
                            dynamic_cast<QEFIDevicePathHardwareController *>(dp);
                    list << qMakePair<QString, QString>("Controller",
                        QString::number(dpController->controller()));
                }
                break;
            case QEFIDevicePathHardwareSubType::HW_BMC:
                if (dynamic_cast<QEFIDevicePathHardwareBMC *>(dp)) {
                    QEFIDevicePathHardwareBMC *dpBMC = (QEFIDevicePathHardwareBMC *)
                        dynamic_cast<QEFIDevicePathHardwareBMC *>(dp);
                    list << qMakePair<QString, QString>("Interface Type",
                        QString::number(dpBMC->interfaceType()));
                    list << qMakePair<QString, QString>("Base Address",
                        QString::number(dpBMC->baseAddress()));
                }
                break;
        }
        break;
    case QEFIDevicePathType::DP_ACPI:
        switch (subtype) {
            case QEFIDevicePathACPISubType::ACPI_HID:
                if (dynamic_cast<QEFIDevicePathACPIHID *>(dp)) {
                    QEFIDevicePathACPIHID *dpACPIHID = (QEFIDevicePathACPIHID *)
                        dynamic_cast<QEFIDevicePathACPIHID *>(dp);
                    list << qMakePair<QString, QString>("HID",
                        QString::number(dpACPIHID->hid()));
                    list << qMakePair<QString, QString>("UID",
                        QString::number(dpACPIHID->uid()));
                }
                break;
            case QEFIDevicePathACPISubType::ACPI_HIDEX:
                if (dynamic_cast<QEFIDevicePathACPIHIDEX *>(dp)) {
                    QEFIDevicePathACPIHIDEX *dpACPIHIDEX =
                        (QEFIDevicePathACPIHIDEX *)
                            dynamic_cast<QEFIDevicePathACPIHIDEX *>(dp);
                    list << qMakePair<QString, QString>("HID",
                        QString::number(dpACPIHIDEX->hid()));
                    list << qMakePair<QString, QString>("UID",
                        QString::number(dpACPIHIDEX->uid()));
                    list << qMakePair<QString, QString>("CID",
                        QString::number(dpACPIHIDEX->cid()));
                    list << qMakePair<QString, QString>("HID String",
                        dpACPIHIDEX->hidString());
                    list << qMakePair<QString, QString>("UID String",
                        dpACPIHIDEX->uidString());
                    list << qMakePair<QString, QString>("CID String",
                        dpACPIHIDEX->cidString());
                }
                break;
            case QEFIDevicePathACPISubType::ACPI_ADR:
                if (dynamic_cast<QEFIDevicePathACPIADR *>(dp)) {
                    QEFIDevicePathACPIADR *dpADR = (QEFIDevicePathACPIADR *)
                        dynamic_cast<QEFIDevicePathACPIADR *>(dp);
                    QList<quint32> addrs = dpADR->addresses();
                    for (int i = 0; i < addrs.size(); i++) {
                        list << qMakePair<QString, QString>(
                            QStringLiteral("Address %1:").arg(i + 1),
                            QString::number(addrs[i])
                        );
                    }
                }
                break;
        }
        break;
    case QEFIDevicePathType::DP_Message:
        switch (subtype) {
            case QEFIDevicePathMessageSubType::MSG_ATAPI:
                if (dynamic_cast<QEFIDevicePathMessageATAPI *>(dp)) {
                    QEFIDevicePathMessageATAPI *dpMessage =
                        (QEFIDevicePathMessageATAPI *)
                            dynamic_cast<QEFIDevicePathMessageATAPI *>(dp);
                    list << qMakePair<QString, QString>("Primary",
                        QString::number(dpMessage->primary()));
                    list << qMakePair<QString, QString>("Slave",
                        QString::number(dpMessage->slave()));
                    list << qMakePair<QString, QString>("Lun",
                        QString::number(dpMessage->lun()));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_SCSI:
                if (dynamic_cast<QEFIDevicePathMessageSCSI *>(dp)) {
                    QEFIDevicePathMessageSCSI *dpMessage =
                        (QEFIDevicePathMessageSCSI *)
                            dynamic_cast<QEFIDevicePathMessageSCSI *>(dp);
                    list << qMakePair<QString, QString>("Target",
                        QString::number(dpMessage->target()));
                    list << qMakePair<QString, QString>("Lun",
                        QString::number(dpMessage->lun()));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_FibreChan:
                if (dynamic_cast<QEFIDevicePathMessageFibreChan *>(dp)) {
                    QEFIDevicePathMessageFibreChan *dpMessage =
                        (QEFIDevicePathMessageFibreChan *)
                            dynamic_cast<QEFIDevicePathMessageFibreChan *>(dp);
                    list << qMakePair<QString, QString>("WWN",
                        QString::number(dpMessage->wwn()));
                    list << qMakePair<QString, QString>("Lun",
                        QString::number(dpMessage->lun()));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_1394:
                if (dynamic_cast<QEFIDevicePathMessage1394 *>(dp)) {
                    QEFIDevicePathMessage1394 *dpMessage =
                        (QEFIDevicePathMessage1394 *)
                            dynamic_cast<QEFIDevicePathMessage1394 *>(dp);
                    list << qMakePair<QString, QString>("GUID",
                        QString::number(dpMessage->guid()));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_USB:
                if (dynamic_cast<QEFIDevicePathMessageUSB *>(dp)) {
                    QEFIDevicePathMessageUSB *dpMessage =
                        (QEFIDevicePathMessageUSB *)
                            dynamic_cast<QEFIDevicePathMessageUSB *>(dp);
                    list << qMakePair<QString, QString>("Parent Port",
                        QString::number(dpMessage->parentPort()));
                    list << qMakePair<QString, QString>("Interface",
                        QString::number(dpMessage->usbInterface()));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_I2O:
                if (dynamic_cast<QEFIDevicePathMessageI2O *>(dp)) {
                    QEFIDevicePathMessageI2O *dpMessage =
                        (QEFIDevicePathMessageI2O *)
                            dynamic_cast<QEFIDevicePathMessageI2O *>(dp);
                    list << qMakePair<QString, QString>("Target",
                        QString::number(dpMessage->target()));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_InfiniBand:
                if (dynamic_cast<QEFIDevicePathMessageInfiniBand *>(dp)) {
                    QEFIDevicePathMessageInfiniBand *dpMessage =
                        (QEFIDevicePathMessageInfiniBand *)
                            dynamic_cast<QEFIDevicePathMessageInfiniBand *>(dp);
                    list << qMakePair<QString, QString>("Resource",
                        QString::number(dpMessage->resourceFlags()));
                    list << qMakePair<QString, QString>("Port GID 1",
                        QString::number(dpMessage->portGID1()));
                    list << qMakePair<QString, QString>("Port GID 2",
                        QString::number(dpMessage->portGID2()));
                    list << qMakePair<QString, QString>("Shared",
                        QString::number(dpMessage->serviceID()));
                    list << qMakePair<QString, QString>("Target Port ID",
                        QString::number(dpMessage->targetPortID()));
                    list << qMakePair<QString, QString>("Device ID",
                        QString::number(dpMessage->deviceID()));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_Vendor:
                if (dynamic_cast<QEFIDevicePathMessageVendor *>(dp)) {
                    QEFIDevicePathMessageVendor *dpMessage =
                        (QEFIDevicePathMessageVendor *)
                            dynamic_cast<QEFIDevicePathMessageVendor *>(dp);
                    list << qMakePair<QString, QString>("GUID",
                        dpMessage->vendorGuid().toString());
                    QByteArray data = dpMessage->vendorData();
                    if (data.size() > DISPLAY_DATA_LIMIT * 2) {
                        data.truncate(DISPLAY_DATA_LIMIT * 2);
                        data += "...";
                    }
                    list << qMakePair<QString, QString>("Data", data);
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_MACAddr:
                if (dynamic_cast<QEFIDevicePathMessageMACAddr *>(dp)) {
                    QEFIDevicePathMessageMACAddr *dpMessage =
                        (QEFIDevicePathMessageMACAddr *)
                            dynamic_cast<QEFIDevicePathMessageMACAddr *>(dp);
                    list << qMakePair<QString, QString>("Interface Type",
                        QString::number(dpMessage->interfaceType()));
                    QByteArray addr((const char*)
                        (dpMessage->macAddress().address), 32);
                    list << qMakePair<QString, QString>("MAC Address",
                        addr.toHex(':'));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_IPv4:
                if (dynamic_cast<QEFIDevicePathMessageIPv4Addr *>(dp)) {
                    QEFIDevicePathMessageIPv4Addr *dpMessage =
                        (QEFIDevicePathMessageIPv4Addr *)
                            dynamic_cast<QEFIDevicePathMessageIPv4Addr *>(dp);
                    QEFIIPv4Address local = dpMessage->localIPv4Address();
                    list << qMakePair<QString, QString>("Local IP",
                        convert_ipv4_to_string(&local));
                    QEFIIPv4Address remote = dpMessage->remoteIPv4Address();
                    list << qMakePair<QString, QString>("Remote IP",
                        convert_ipv4_to_string(&remote));
                    list << qMakePair<QString, QString>("Local Port",
                        QString::number(dpMessage->localPort()));
                    list << qMakePair<QString, QString>("Remote Port",
                        QString::number(dpMessage->remotePort()));
                    list << qMakePair<QString, QString>("Protocol",
                        QString::number(dpMessage->protocol()));
                    list << qMakePair<QString, QString>("Static Address",
                        dpMessage->staticIPAddress() ? "Yes" : " No");
                    QEFIIPv4Address gw = dpMessage->gateway();
                    list << qMakePair<QString, QString>("Gateway IP",
                        convert_ipv4_to_string(&gw));
                    QEFIIPv4Address netmask = dpMessage->netmask();
                    list << qMakePair<QString, QString>("Netmask",
                        convert_ipv4_to_string(&netmask));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_IPv6:
                if (dynamic_cast<QEFIDevicePathMessageIPv6Addr *>(dp)) {
                    QEFIDevicePathMessageIPv6Addr *dpMessage =
                        (QEFIDevicePathMessageIPv6Addr *)
                            dynamic_cast<QEFIDevicePathMessageIPv6Addr *>(dp);
                    QEFIIPv6Address local = dpMessage->localIPv6Address();
                    list << qMakePair<QString, QString>("Local IP",
                        QHostAddress(local.address).toString());
                    QEFIIPv6Address remote = dpMessage->remoteIPv6Address();
                    list << qMakePair<QString, QString>("Remote IP",
                        QHostAddress(remote.address).toString());
                    list << qMakePair<QString, QString>("Local Port",
                        QString::number(dpMessage->localPort()));
                    list << qMakePair<QString, QString>("Remote Port",
                        QString::number(dpMessage->remotePort()));
                    list << qMakePair<QString, QString>("Protocol",
                        QString::number(dpMessage->protocol()));
                    list << qMakePair<QString, QString>("IP Address Origin",
                        QString::number(dpMessage->ipAddressOrigin()));
                    list << qMakePair<QString, QString>("Prefix Length",
                        QString::number(dpMessage->prefixLength()));
                    list << qMakePair<QString, QString>("Gateway IP",
                        QString::number(dpMessage->gatewayIPv6Address()));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_UART:
                if (dynamic_cast<QEFIDevicePathMessageUART *>(dp)) {
                    QEFIDevicePathMessageUART *dpMessage =
                        (QEFIDevicePathMessageUART *)
                            dynamic_cast<QEFIDevicePathMessageUART *>(dp);
                    list << qMakePair<QString, QString>("Baud Rate",
                        QString::number(dpMessage->baudRate()));
                    list << qMakePair<QString, QString>("Data Bits",
                        QString::number(dpMessage->dataBits()));
                    list << qMakePair<QString, QString>("Parity",
                        QString::number(dpMessage->parity()));
                    list << qMakePair<QString, QString>("Stop Bits",
                        QString::number(dpMessage->stopBits()));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_USBClass:
                if (dynamic_cast<QEFIDevicePathMessageUSBClass *>(dp)) {
                    QEFIDevicePathMessageUSBClass *dpMessage =
                        (QEFIDevicePathMessageUSBClass *)
                            dynamic_cast<QEFIDevicePathMessageUSBClass *>(dp);
                    list << qMakePair<QString, QString>("VID",
                        QString::number(dpMessage->vendorId()));
                    list << qMakePair<QString, QString>("PID",
                        QString::number(dpMessage->productId()));
                    list << qMakePair<QString, QString>("Device Class",
                        QString::number(dpMessage->deviceClass()));
                    list << qMakePair<QString, QString>("Device Subclass",
                        QString::number(dpMessage->deviceSubclass()));
                    list << qMakePair<QString, QString>("Device Protocol",
                        QString::number(dpMessage->deviceProtocol()));
                }
                break;

            case QEFIDevicePathMessageSubType::MSG_USBWWID:
                if (dynamic_cast<QEFIDevicePathMessageUSBWWID *>(dp)) {
                    QEFIDevicePathMessageUSBWWID *dpMessage =
                        (QEFIDevicePathMessageUSBWWID *)
                            dynamic_cast<QEFIDevicePathMessageUSBWWID *>(dp);
                    list << qMakePair<QString, QString>("VID",
                        QString::number(dpMessage->vendorId()));
                    list << qMakePair<QString, QString>("PID",
                        QString::number(dpMessage->productId()));
                    // TODO: Serial number
                }
                break;

            case QEFIDevicePathMessageSubType::MSG_LUN:
                if (dynamic_cast<QEFIDevicePathMessageLUN *>(dp)) {
                    QEFIDevicePathMessageLUN *dpMessage =
                        (QEFIDevicePathMessageLUN *)
                            dynamic_cast<QEFIDevicePathMessageLUN *>(dp);
                    list << qMakePair<QString, QString>("Lun",
                        QString::number(dpMessage->lun()));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_SATA:
                if (dynamic_cast<QEFIDevicePathMessageSATA *>(dp)) {
                    QEFIDevicePathMessageSATA *dpMessage =
                        (QEFIDevicePathMessageSATA *)
                            dynamic_cast<QEFIDevicePathMessageSATA *>(dp);
                    list << qMakePair<QString, QString>("HBA Port",
                        QString::number(dpMessage->hbaPort()));
                    list << qMakePair<QString, QString>("Port Multiplier",
                        QString::number(dpMessage->portMultiplierPort()));
                    list << qMakePair<QString, QString>("Lun",
                        QString::number(dpMessage->lun()));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_ISCSI:
                if (dynamic_cast<QEFIDevicePathMessageISCSI *>(dp)) {
                    QEFIDevicePathMessageISCSI *dpMessage =
                        (QEFIDevicePathMessageISCSI *)
                            dynamic_cast<QEFIDevicePathMessageISCSI *>(dp);
                    list << qMakePair<QString, QString>("Protocol",
                        QString::number(dpMessage->protocol()));
                    list << qMakePair<QString, QString>("Options",
                        QString::number(dpMessage->options()));
                    QByteArray lun((const char*)
                        (dpMessage->lun().data), 8);
                    list << qMakePair<QString, QString>("Lun",
                        lun.toHex(':'));
                    list << qMakePair<QString, QString>("TGPT",
                        QString::number(dpMessage->tpgt()));
                    list << qMakePair<QString, QString>("Target",
                        dpMessage->targetName());
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_VLAN:
                if (dynamic_cast<QEFIDevicePathMessageVLAN *>(dp)) {
                    QEFIDevicePathMessageVLAN *dpMessage =
                        (QEFIDevicePathMessageVLAN *)
                            dynamic_cast<QEFIDevicePathMessageVLAN *>(dp);
                    list << qMakePair<QString, QString>("VLAN ID",
                        QString::number(dpMessage->vlanID()));
                }
                break;

            case QEFIDevicePathMessageSubType::MSG_FibreChanEx:
                if (dynamic_cast<QEFIDevicePathMessageFibreChanEx *>(dp)) {
                    QEFIDevicePathMessageFibreChanEx *dpMessage =
                        (QEFIDevicePathMessageFibreChanEx *)
                            dynamic_cast<QEFIDevicePathMessageFibreChanEx *>(dp);
                    QByteArray wwn((const char*)
                        (dpMessage->lun().data), 8);
                    list << qMakePair<QString, QString>("WWN",
                        wwn.toHex(':'));
                    QByteArray lun((const char*)
                        (dpMessage->lun().data), 8);
                    list << qMakePair<QString, QString>("Lun",
                        lun.toHex(':'));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_SASEX:
                if (dynamic_cast<QEFIDevicePathMessageSASEx *>(dp)) {
                    QEFIDevicePathMessageSASEx *dpMessage =
                        (QEFIDevicePathMessageSASEx *)
                            dynamic_cast<QEFIDevicePathMessageSASEx *>(dp);
                    QByteArray sasAddr((const char*)
                        (dpMessage->sasAddress().address), 8);
                    list << qMakePair<QString, QString>("SAS Address",
                        sasAddr.toHex(':'));
                    QByteArray lun((const char*)
                        (dpMessage->lun().data), 8);
                    list << qMakePair<QString, QString>("Lun",
                        sasAddr.toHex(':'));
                    list << qMakePair<QString, QString>("Device Topology",
                        QString::number(dpMessage->deviceTopologyInfo()));
                    list << qMakePair<QString, QString>("Drive Bay ID",
                        QString::number(dpMessage->driveBayID()));
                    list << qMakePair<QString, QString>("RTP",
                        QString::number(dpMessage->rtp()));
                }
                break;

            case QEFIDevicePathMessageSubType::MSG_NVME:
                if (dynamic_cast<QEFIDevicePathMessageNVME *>(dp)) {
                    QEFIDevicePathMessageNVME *dpMessage =
                        (QEFIDevicePathMessageNVME *)
                            dynamic_cast<QEFIDevicePathMessageNVME *>(dp);
                    list << qMakePair<QString, QString>("Namespace ID",
                        QString::number(dpMessage->namespaceID()));
                    QByteArray eui((const char*)
                        (dpMessage->ieeeEui64().eui), 8);
                    list << qMakePair<QString, QString>("EUI",
                        eui.toHex(':'));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_URI:
                if (dynamic_cast<QEFIDevicePathMessageURI *>(dp)) {
                    QEFIDevicePathMessageURI *dpMessage =
                        (QEFIDevicePathMessageURI *)
                            dynamic_cast<QEFIDevicePathMessageURI *>(dp);
                    list << qMakePair<QString, QString>("URI",
                        dpMessage->uri().toString());
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_UFS:
                if (dynamic_cast<QEFIDevicePathMessageUFS *>(dp)) {
                    QEFIDevicePathMessageUFS *dpMessage =
                        (QEFIDevicePathMessageUFS *)
                            dynamic_cast<QEFIDevicePathMessageUFS *>(dp);
                    list << qMakePair<QString, QString>("Target ID",
                        QString::number(dpMessage->targetID()));
                    list << qMakePair<QString, QString>("Lun",
                        QString::number(dpMessage->lun()));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_SD:
                if (dynamic_cast<QEFIDevicePathMessageSD *>(dp)) {
                    QEFIDevicePathMessageSD *dpMessage =
                        (QEFIDevicePathMessageSD *)
                            dynamic_cast<QEFIDevicePathMessageSD *>(dp);
                    list << qMakePair<QString, QString>("Slot",
                        QString::number(dpMessage->slotNumber()));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_BT:
                if (dynamic_cast<QEFIDevicePathMessageBT *>(dp)) {
                    QEFIDevicePathMessageBT *dpMessage =
                        (QEFIDevicePathMessageBT *)
                            dynamic_cast<QEFIDevicePathMessageBT *>(dp);
                    QByteArray addr((const char*)
                        (dpMessage->address().address), 6);
                    list << qMakePair<QString, QString>("MAC Address",
                        addr.toHex(':'));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_WiFi:
                if (dynamic_cast<QEFIDevicePathMessageWiFi *>(dp)) {
                    QEFIDevicePathMessageWiFi *dpMessage =
                        (QEFIDevicePathMessageWiFi *)
                            dynamic_cast<QEFIDevicePathMessageWiFi *>(dp);
                    list << qMakePair<QString, QString>("SSID",
                        dpMessage->ssid());
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_EMMC:
                if (dynamic_cast<QEFIDevicePathMessageEMMC *>(dp)) {
                    QEFIDevicePathMessageEMMC *dpMessage =
                        (QEFIDevicePathMessageEMMC *)
                            dynamic_cast<QEFIDevicePathMessageEMMC *>(dp);
                    list << qMakePair<QString, QString>("Slot",
                        QString::number(dpMessage->slotNumber()));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_BTLE:
                if (dynamic_cast<QEFIDevicePathMessageBTLE *>(dp)) {
                    QEFIDevicePathMessageBTLE *dpMessage =
                        (QEFIDevicePathMessageBTLE *)
                            dynamic_cast<QEFIDevicePathMessageBTLE *>(dp);
                    QByteArray addr((const char*)
                        (dpMessage->address().address), 6);
                    list << qMakePair<QString, QString>("MAC Address",
                        addr.toHex(':'));
                    list << qMakePair<QString, QString>("Address Type",
                        QString::number(dpMessage->addressType()));
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_DNS:
                if (dynamic_cast<QEFIDevicePathMessageDNS *>(dp)) {
                    QEFIDevicePathMessageDNS *dpMessage =
                        (QEFIDevicePathMessageDNS *)
                            dynamic_cast<QEFIDevicePathMessageDNS *>(dp);
                    // TODO: DNS Addresses
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_NVDIMM:
                if (dynamic_cast<QEFIDevicePathMessageNVDIMM *>(dp)) {
                    QEFIDevicePathMessageNVDIMM *dpMessage =
                        (QEFIDevicePathMessageNVDIMM *)
                            dynamic_cast<QEFIDevicePathMessageNVDIMM *>(dp);
                    list << qMakePair<QString, QString>("UUID",
                        dpMessage->uuid().toString());
                }
                break;
        }
        break;
    case QEFIDevicePathType::DP_Media:
        switch (subtype) {
            case QEFIDevicePathMediaSubType::MEDIA_HD:
                if (dynamic_cast<QEFIDevicePathMediaHD *>(dp)) {
                    QEFIDevicePathMediaHD *dpHD = (QEFIDevicePathMediaHD *)
                        dynamic_cast<QEFIDevicePathMediaHD *>(dp);
                    list << qMakePair<QString, QString>("Partition Num",
                        QString::number(dpHD->partitionNumber()));
                    list << qMakePair<QString, QString>("Start",
                        QString::number(dpHD->start()));
                    list << qMakePair<QString, QString>("Size",
                        QString::number(dpHD->size()));
                    switch (dpHD->format()) {
                        case QEFIDevicePathMediaHD::
                            QEFIDevicePathMediaHDFormat::PCAT:
                            list << qMakePair<QString, QString>("Format", "PCAT");
                        break;
                        case QEFIDevicePathMediaHD::
                            QEFIDevicePathMediaHDFormat::GPT:
                            list << qMakePair<QString, QString>("Format", "GPT");
                        break;
                        default:
                            list << qMakePair<QString, QString>("Format", "Unknown");
                        break;
                    }

                    switch (dpHD->signatureType())
                    {
                        case QEFIDevicePathMediaHD::
                            QEFIDevicePathMediaHDSignatureType::MBR:
                            list << qMakePair<QString, QString>("MBR",
                                QString::number(dpHD->mbrSignature()));
                            break;
                        case QEFIDevicePathMediaHD::
                            QEFIDevicePathMediaHDSignatureType::GUID:
                            list << qMakePair<QString, QString>("GUID",
                                dpHD->gptGuid().toString());
                            break;
                        default:
                            break;
                    }
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_File:
                if (dynamic_cast<QEFIDevicePathMediaFile *>(dp)) {
                    QEFIDevicePathMediaFile *dpFile = (QEFIDevicePathMediaFile *)
                        dynamic_cast<QEFIDevicePathMediaFile *>(dp);
                    list << qMakePair<QString, QString>("File", dpFile->name());
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_CDROM:
                if (dynamic_cast<QEFIDevicePathMediaCDROM *>(dp)) {
                    QEFIDevicePathMediaCDROM *dpCDROM = (QEFIDevicePathMediaCDROM *)
                        dynamic_cast<QEFIDevicePathMediaCDROM *>(dp);
                    list << qMakePair<QString, QString>("Boot Catalogue Entry",
                        QString::number(dpCDROM->bootCatalogEntry()));
                    list << qMakePair<QString, QString>("Partition RBA",
                        QString::number(dpCDROM->partitionRba()));
                    list << qMakePair<QString, QString>("Sectors",
                        QString::number(dpCDROM->sectors()));
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_Vendor:
                if (dynamic_cast<QEFIDevicePathMediaVendor *>(dp)) {
                    QEFIDevicePathMediaVendor *dpVendor = (QEFIDevicePathMediaVendor *)
                        dynamic_cast<QEFIDevicePathMediaVendor *>(dp);
                    list << qMakePair<QString, QString>("GUID",
                        dpVendor->vendorGuid().toString());
                    QByteArray data = dpVendor->vendorData().toHex();
                    if (data.size() > DISPLAY_DATA_LIMIT * 2) {
                        data.truncate(DISPLAY_DATA_LIMIT * 2);
                        data += "...";
                    }
                    list << qMakePair<QString, QString>("Data", data);
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_Protocol:
                if (dynamic_cast<QEFIDevicePathMediaProtocol *>(dp)) {
                    QEFIDevicePathMediaProtocol *dpProtocol = (QEFIDevicePathMediaProtocol *)
                        dynamic_cast<QEFIDevicePathMediaProtocol *>(dp);
                    list << qMakePair<QString, QString>("Protocol GUID",
                        dpProtocol->protocolGuid().toString());
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_FirmwareFile:
                if (dynamic_cast<QEFIDevicePathMediaFirmwareFile *>(dp)) {
                    QEFIDevicePathMediaFirmwareFile *dpFile =
                        (QEFIDevicePathMediaFirmwareFile *)
                        dynamic_cast<QEFIDevicePathMediaFirmwareFile *>(dp);

                    QByteArray data = dpFile->piInfo().toHex();
                    if (data.size() > DISPLAY_DATA_LIMIT * 2) {
                        data.truncate(DISPLAY_DATA_LIMIT * 2);
                        data += "...";
                    }
                    list << qMakePair<QString, QString>("PI Info", data);
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_FirmwareVolume:
                if (dynamic_cast<QEFIDevicePathMediaFile *>(dp)) {
                    QEFIDevicePathMediaFirmwareVolume *dpFV =
                        (QEFIDevicePathMediaFirmwareVolume *)
                        dynamic_cast<QEFIDevicePathMediaFirmwareVolume *>(dp);
                    QByteArray data = dpFV->piInfo().toHex();
                    if (data.size() > DISPLAY_DATA_LIMIT * 2) {
                        data.truncate(DISPLAY_DATA_LIMIT * 2);
                        data += "...";
                    }
                    list << qMakePair<QString, QString>("PI Info", data);
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_RelativeOffset:
                if (dynamic_cast<QEFIDevicePathMediaRelativeOffset *>(dp)) {
                    QEFIDevicePathMediaRelativeOffset *dpRO =
                        (QEFIDevicePathMediaRelativeOffset *)
                        dynamic_cast<QEFIDevicePathMediaRelativeOffset *>(dp);
                    list << qMakePair<QString, QString>("First Byte",
                        QString::number(dpRO->firstByte()));
                    list << qMakePair<QString, QString>("Last Byte",
                        QString::number(dpRO->lastByte()));
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_RamDisk:
                if (dynamic_cast<QEFIDevicePathMediaRAMDisk *>(dp)) {
                    QEFIDevicePathMediaRAMDisk *dpRAMDisk =
                        (QEFIDevicePathMediaRAMDisk *)
                        dynamic_cast<QEFIDevicePathMediaRAMDisk *>(dp);
                    list << qMakePair<QString, QString>("Start",
                        QString::number(dpRAMDisk->startAddress()));
                    list << qMakePair<QString, QString>("End",
                        QString::number(dpRAMDisk->endAddress()));
                    list << qMakePair<QString, QString>("Disk Type Guid",
                        dpRAMDisk->diskTypeGuid().toString());
                    list << qMakePair<QString, QString>("Instance Number",
                        QString::number(dpRAMDisk->instanceNumber()));
                }
                break;
        }
        break;
    case QEFIDevicePathType::DP_BIOSBoot:
        if (dynamic_cast<QEFIDevicePathBIOSBoot *>(dp)) {
            QEFIDevicePathBIOSBoot *dpBIOSBoot = (QEFIDevicePathBIOSBoot *)
                dynamic_cast<QEFIDevicePathBIOSBoot *>(dp);
            QByteArray description = dpBIOSBoot->description().toHex();
            list << qMakePair<QString, QString>("Device Type",
                QString::number(dpBIOSBoot->deviceType()));
            list << qMakePair<QString, QString>("Status",
                QString::number(dpBIOSBoot->status()));

            if (description.size() > DISPLAY_DATA_LIMIT * 2) {
                description.truncate(DISPLAY_DATA_LIMIT * 2);
                description += "...";
            }
            list << qMakePair<QString, QString>("Description", description);
        }
        break;
    }
    return list;
}

QList<QPair<QString, enum QEFIDPEditType>> convert_device_path_types(QEFIDevicePath *dp)
{
    if (dp == nullptr) return {};
    QList<QPair<QString, enum QEFIDPEditType>> list;
    QEFIDevicePathType type = dp->type();
    quint8 subtype = dp->subType();
    switch (type)
    {
    case QEFIDevicePathType::DP_Hardware:
        switch (subtype) {
            case QEFIDevicePathHardwareSubType::HW_PCI:
                if (dynamic_cast<QEFIDevicePathHardwarePCI *>(dp)) {
                    QEFIDevicePathHardwarePCI *dpPCI = (QEFIDevicePathHardwarePCI *)
                        dynamic_cast<QEFIDevicePathHardwarePCI *>(dp);
                    list << qMakePair<QString, enum QEFIDPEditType>("Function",
                        QEFIDPEditType::EditType_HexNumber);
                    list << qMakePair<QString, enum QEFIDPEditType>("Device",
                        QEFIDPEditType::EditType_HexNumber);
                }
                break;
            case QEFIDevicePathHardwareSubType::HW_PCCard:
                if (dynamic_cast<QEFIDevicePathHardwarePCCard *>(dp)) {
                    QEFIDevicePathHardwarePCCard *dpPCCard = (QEFIDevicePathHardwarePCCard *)
                        dynamic_cast<QEFIDevicePathHardwarePCCard *>(dp);
                    list << qMakePair<QString, enum QEFIDPEditType>("Function",
                        QEFIDPEditType::EditType_HexNumber);
                }
                break;
            case QEFIDevicePathHardwareSubType::HW_MMIO:
                if (dynamic_cast<QEFIDevicePathHardwareMMIO *>(dp)) {
                    QEFIDevicePathHardwareMMIO *dpMMIO = (QEFIDevicePathHardwareMMIO *)
                        dynamic_cast<QEFIDevicePathHardwareMMIO *>(dp);
                    list << qMakePair<QString, enum QEFIDPEditType>("Memory Type",
                        QEFIDPEditType::EditType_Number);   // TODO: Enum
                    list << qMakePair<QString, enum QEFIDPEditType>("Starting Address",
                        QEFIDPEditType::EditType_HexNumber);
                    list << qMakePair<QString, enum QEFIDPEditType>("Ending Address",
                        QEFIDPEditType::EditType_HexNumber);
                }
                break;
            case QEFIDevicePathHardwareSubType::HW_Vendor:
                if (dynamic_cast<QEFIDevicePathHardwareVendor *>(dp)) {
                    QEFIDevicePathHardwareVendor *dpVendor = (QEFIDevicePathHardwareVendor *)
                        dynamic_cast<QEFIDevicePathHardwareVendor *>(dp);
                    list << qMakePair<QString, enum QEFIDPEditType>("GUID",
                        QEFIDPEditType::EditType_UUID);
                    list << qMakePair<QString, enum QEFIDPEditType>("Data",
                        QEFIDPEditType::EditType_HexData);
                }
                break;
            case QEFIDevicePathHardwareSubType::HW_Controller:
                if (dynamic_cast<QEFIDevicePathHardwareController *>(dp)) {
                    QEFIDevicePathHardwareController *dpController =
                        (QEFIDevicePathHardwareController *)
                            dynamic_cast<QEFIDevicePathHardwareController *>(dp);
                    list << qMakePair<QString, enum QEFIDPEditType>("Controller",
                        QEFIDPEditType::EditType_HexNumber);
                }
                break;
            case QEFIDevicePathHardwareSubType::HW_BMC:
                if (dynamic_cast<QEFIDevicePathHardwareBMC *>(dp)) {
                    QEFIDevicePathHardwareBMC *dpBMC = (QEFIDevicePathHardwareBMC *)
                        dynamic_cast<QEFIDevicePathHardwareBMC *>(dp);
                    list << qMakePair<QString, enum QEFIDPEditType>("Interface Type",
                        QEFIDPEditType::EditType_Number);   // TODO: Enum
                    list << qMakePair<QString, enum QEFIDPEditType>("Base Address",
                        QEFIDPEditType::EditType_HexNumber);
                }
                break;
        }
        break;
    case QEFIDevicePathType::DP_ACPI:
        switch (subtype) {
            case QEFIDevicePathACPISubType::ACPI_HID:
                if (dynamic_cast<QEFIDevicePathACPIHID *>(dp)) {
                    QEFIDevicePathACPIHID *dpACPIHID = (QEFIDevicePathACPIHID *)
                        dynamic_cast<QEFIDevicePathACPIHID *>(dp);
                    list << qMakePair<QString, enum QEFIDPEditType>("HID",
                        QEFIDPEditType::EditType_HexNumber);
                    list << qMakePair<QString, enum QEFIDPEditType>("UID",
                        QEFIDPEditType::EditType_HexNumber);
                }
                break;
            case QEFIDevicePathACPISubType::ACPI_HIDEX:
                if (dynamic_cast<QEFIDevicePathACPIHIDEX *>(dp)) {
                    QEFIDevicePathACPIHIDEX *dpACPIHIDEX =
                        (QEFIDevicePathACPIHIDEX *)
                            dynamic_cast<QEFIDevicePathACPIHIDEX *>(dp);
                    list << qMakePair<QString, enum QEFIDPEditType>("HID",
                        QEFIDPEditType::EditType_HexNumber);
                    list << qMakePair<QString, enum QEFIDPEditType>("UID",
                        QEFIDPEditType::EditType_HexNumber);
                    list << qMakePair<QString, enum QEFIDPEditType>("CID",
                        QEFIDPEditType::EditType_HexNumber);
                    list << qMakePair<QString, enum QEFIDPEditType>("HID String",
                        QEFIDPEditType::EditType_Text);
                    list << qMakePair<QString, enum QEFIDPEditType>("UID String",
                        QEFIDPEditType::EditType_Text);
                    list << qMakePair<QString, enum QEFIDPEditType>("CID String",
                        QEFIDPEditType::EditType_Text);
                }
                break;
            case QEFIDevicePathACPISubType::ACPI_ADR:
                if (dynamic_cast<QEFIDevicePathACPIADR *>(dp)) {
                    QEFIDevicePathACPIADR *dpADR = (QEFIDevicePathACPIADR *)
                        dynamic_cast<QEFIDevicePathACPIADR *>(dp);
                    // TODO: List not yet supported
                }
                break;
        }
        break;
    case QEFIDevicePathType::DP_Message:
        switch (subtype) {
            case QEFIDevicePathMessageSubType::MSG_ATAPI:
                if (dynamic_cast<QEFIDevicePathMessageATAPI *>(dp)) {
                    QEFIDevicePathMessageATAPI *dpMessage =
                        (QEFIDevicePathMessageATAPI *)
                            dynamic_cast<QEFIDevicePathMessageATAPI *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_SCSI:
                if (dynamic_cast<QEFIDevicePathMessageSCSI *>(dp)) {
                    QEFIDevicePathMessageSCSI *dpMessage =
                        (QEFIDevicePathMessageSCSI *)
                            dynamic_cast<QEFIDevicePathMessageSCSI *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_FibreChan:
                if (dynamic_cast<QEFIDevicePathMessageFibreChan *>(dp)) {
                    QEFIDevicePathMessageFibreChan *dpMessage =
                        (QEFIDevicePathMessageFibreChan *)
                            dynamic_cast<QEFIDevicePathMessageFibreChan *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_1394:
                if (dynamic_cast<QEFIDevicePathMessage1394 *>(dp)) {
                    QEFIDevicePathMessage1394 *dpMessage =
                        (QEFIDevicePathMessage1394 *)
                            dynamic_cast<QEFIDevicePathMessage1394 *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_USB:
                if (dynamic_cast<QEFIDevicePathMessageUSB *>(dp)) {
                    QEFIDevicePathMessageUSB *dpMessage =
                        (QEFIDevicePathMessageUSB *)
                            dynamic_cast<QEFIDevicePathMessageUSB *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_I2O:
                if (dynamic_cast<QEFIDevicePathMessageI2O *>(dp)) {
                    QEFIDevicePathMessageI2O *dpMessage =
                        (QEFIDevicePathMessageI2O *)
                            dynamic_cast<QEFIDevicePathMessageI2O *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_InfiniBand:
                if (dynamic_cast<QEFIDevicePathMessageInfiniBand *>(dp)) {
                    QEFIDevicePathMessageInfiniBand *dpMessage =
                        (QEFIDevicePathMessageInfiniBand *)
                            dynamic_cast<QEFIDevicePathMessageInfiniBand *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_Vendor:
                if (dynamic_cast<QEFIDevicePathMessageVendor *>(dp)) {
                    QEFIDevicePathMessageVendor *dpMessage =
                        (QEFIDevicePathMessageVendor *)
                            dynamic_cast<QEFIDevicePathMessageVendor *>(dp);
                    list << qMakePair<QString, enum QEFIDPEditType>("GUID",
                        QEFIDPEditType::EditType_UUID);
                    list << qMakePair<QString, enum QEFIDPEditType>("Data",
                        QEFIDPEditType::EditType_HexData);
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_MACAddr:
                if (dynamic_cast<QEFIDevicePathMessageMACAddr *>(dp)) {
                    QEFIDevicePathMessageMACAddr *dpMessage =
                        (QEFIDevicePathMessageMACAddr *)
                            dynamic_cast<QEFIDevicePathMessageMACAddr *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_IPv4:
                if (dynamic_cast<QEFIDevicePathMessageIPv4Addr *>(dp)) {
                    QEFIDevicePathMessageIPv4Addr *dpMessage =
                        (QEFIDevicePathMessageIPv4Addr *)
                            dynamic_cast<QEFIDevicePathMessageIPv4Addr *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_IPv6:
                if (dynamic_cast<QEFIDevicePathMessageIPv6Addr *>(dp)) {
                    QEFIDevicePathMessageIPv6Addr *dpMessage =
                        (QEFIDevicePathMessageIPv6Addr *)
                            dynamic_cast<QEFIDevicePathMessageIPv6Addr *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_UART:
                if (dynamic_cast<QEFIDevicePathMessageUART *>(dp)) {
                    QEFIDevicePathMessageUART *dpMessage =
                        (QEFIDevicePathMessageUART *)
                            dynamic_cast<QEFIDevicePathMessageUART *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_USBClass:
                if (dynamic_cast<QEFIDevicePathMessageUSBClass *>(dp)) {
                    QEFIDevicePathMessageUSBClass *dpMessage =
                        (QEFIDevicePathMessageUSBClass *)
                            dynamic_cast<QEFIDevicePathMessageUSBClass *>(dp);
                    // TODO
                }
                break;

            case QEFIDevicePathMessageSubType::MSG_USBWWID:
                if (dynamic_cast<QEFIDevicePathMessageUSBWWID *>(dp)) {
                    QEFIDevicePathMessageUSBWWID *dpMessage =
                        (QEFIDevicePathMessageUSBWWID *)
                            dynamic_cast<QEFIDevicePathMessageUSBWWID *>(dp);
                    // TODO
                }
                break;

            case QEFIDevicePathMessageSubType::MSG_LUN:
                if (dynamic_cast<QEFIDevicePathMessageLUN *>(dp)) {
                    QEFIDevicePathMessageLUN *dpMessage =
                        (QEFIDevicePathMessageLUN *)
                            dynamic_cast<QEFIDevicePathMessageLUN *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_SATA:
                if (dynamic_cast<QEFIDevicePathMessageSATA *>(dp)) {
                    QEFIDevicePathMessageSATA *dpMessage =
                        (QEFIDevicePathMessageSATA *)
                            dynamic_cast<QEFIDevicePathMessageSATA *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_ISCSI:
                if (dynamic_cast<QEFIDevicePathMessageISCSI *>(dp)) {
                    QEFIDevicePathMessageISCSI *dpMessage =
                        (QEFIDevicePathMessageISCSI *)
                            dynamic_cast<QEFIDevicePathMessageISCSI *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_VLAN:
                if (dynamic_cast<QEFIDevicePathMessageVLAN *>(dp)) {
                    QEFIDevicePathMessageVLAN *dpMessage =
                        (QEFIDevicePathMessageVLAN *)
                            dynamic_cast<QEFIDevicePathMessageVLAN *>(dp);
                    // TODO
                }
                break;

            case QEFIDevicePathMessageSubType::MSG_FibreChanEx:
                if (dynamic_cast<QEFIDevicePathMessageFibreChanEx *>(dp)) {
                    QEFIDevicePathMessageFibreChanEx *dpMessage =
                        (QEFIDevicePathMessageFibreChanEx *)
                            dynamic_cast<QEFIDevicePathMessageFibreChanEx *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_SASEX:
                if (dynamic_cast<QEFIDevicePathMessageSASEx *>(dp)) {
                    QEFIDevicePathMessageSASEx *dpMessage =
                        (QEFIDevicePathMessageSASEx *)
                            dynamic_cast<QEFIDevicePathMessageSASEx *>(dp);
                    // TODO
                }
                break;

            case QEFIDevicePathMessageSubType::MSG_NVME:
                if (dynamic_cast<QEFIDevicePathMessageNVME *>(dp)) {
                    QEFIDevicePathMessageNVME *dpMessage =
                        (QEFIDevicePathMessageNVME *)
                            dynamic_cast<QEFIDevicePathMessageNVME *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_URI:
                if (dynamic_cast<QEFIDevicePathMessageURI *>(dp)) {
                    QEFIDevicePathMessageURI *dpMessage =
                        (QEFIDevicePathMessageURI *)
                            dynamic_cast<QEFIDevicePathMessageURI *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_UFS:
                if (dynamic_cast<QEFIDevicePathMessageUFS *>(dp)) {
                    QEFIDevicePathMessageUFS *dpMessage =
                        (QEFIDevicePathMessageUFS *)
                            dynamic_cast<QEFIDevicePathMessageUFS *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_SD:
                if (dynamic_cast<QEFIDevicePathMessageSD *>(dp)) {
                    QEFIDevicePathMessageSD *dpMessage =
                        (QEFIDevicePathMessageSD *)
                            dynamic_cast<QEFIDevicePathMessageSD *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_BT:
                if (dynamic_cast<QEFIDevicePathMessageBT *>(dp)) {
                    QEFIDevicePathMessageBT *dpMessage =
                        (QEFIDevicePathMessageBT *)
                            dynamic_cast<QEFIDevicePathMessageBT *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_WiFi:
                if (dynamic_cast<QEFIDevicePathMessageWiFi *>(dp)) {
                    QEFIDevicePathMessageWiFi *dpMessage =
                        (QEFIDevicePathMessageWiFi *)
                            dynamic_cast<QEFIDevicePathMessageWiFi *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_EMMC:
                if (dynamic_cast<QEFIDevicePathMessageEMMC *>(dp)) {
                    QEFIDevicePathMessageEMMC *dpMessage =
                        (QEFIDevicePathMessageEMMC *)
                            dynamic_cast<QEFIDevicePathMessageEMMC *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_BTLE:
                if (dynamic_cast<QEFIDevicePathMessageBTLE *>(dp)) {
                    QEFIDevicePathMessageBTLE *dpMessage =
                        (QEFIDevicePathMessageBTLE *)
                            dynamic_cast<QEFIDevicePathMessageBTLE *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_DNS:
                if (dynamic_cast<QEFIDevicePathMessageDNS *>(dp)) {
                    QEFIDevicePathMessageDNS *dpMessage =
                        (QEFIDevicePathMessageDNS *)
                            dynamic_cast<QEFIDevicePathMessageDNS *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMessageSubType::MSG_NVDIMM:
                if (dynamic_cast<QEFIDevicePathMessageNVDIMM *>(dp)) {
                    QEFIDevicePathMessageNVDIMM *dpMessage =
                        (QEFIDevicePathMessageNVDIMM *)
                            dynamic_cast<QEFIDevicePathMessageNVDIMM *>(dp);
                    list << qMakePair<QString, enum QEFIDPEditType>("UUID",
                        QEFIDPEditType::EditType_UUID);
                }
                break;
        }
        break;
    case QEFIDevicePathType::DP_Media:
        switch (subtype) {
            case QEFIDevicePathMediaSubType::MEDIA_HD:
                if (dynamic_cast<QEFIDevicePathMediaHD *>(dp)) {
                    QEFIDevicePathMediaHD *dpHD = (QEFIDevicePathMediaHD *)
                        dynamic_cast<QEFIDevicePathMediaHD *>(dp);
                    list << qMakePair<QString, enum QEFIDPEditType>("Partition Num",
                        QEFIDPEditType::EditType_Number);
                    list << qMakePair<QString, enum QEFIDPEditType>("Start",
                        QEFIDPEditType::EditType_Number);
                    list << qMakePair<QString, enum QEFIDPEditType>("Size",
                        QEFIDPEditType::EditType_Number);
                    list << qMakePair<QString, enum QEFIDPEditType>("Format",
                        QEFIDPEditType::EditType_Enum);
                    list << qMakePair<QString, enum QEFIDPEditType>("Signature Type",
                        QEFIDPEditType::EditType_Enum);
                    list << qMakePair<QString, enum QEFIDPEditType>("Signature",
                        QEFIDPEditType::EditType_HexData);
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_File:
                if (dynamic_cast<QEFIDevicePathMediaFile *>(dp)) {
                    QEFIDevicePathMediaFile *dpFile = (QEFIDevicePathMediaFile *)
                        dynamic_cast<QEFIDevicePathMediaFile *>(dp);
                    list << qMakePair<QString, enum QEFIDPEditType>("File",
                        QEFIDPEditType::EditType_Path);
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_CDROM:
                if (dynamic_cast<QEFIDevicePathMediaCDROM *>(dp)) {
                    QEFIDevicePathMediaCDROM *dpCDROM = (QEFIDevicePathMediaCDROM *)
                        dynamic_cast<QEFIDevicePathMediaCDROM *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_Vendor:
                if (dynamic_cast<QEFIDevicePathMediaVendor *>(dp)) {
                    QEFIDevicePathMediaVendor *dpVendor = (QEFIDevicePathMediaVendor *)
                        dynamic_cast<QEFIDevicePathMediaVendor *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_Protocol:
                if (dynamic_cast<QEFIDevicePathMediaProtocol *>(dp)) {
                    QEFIDevicePathMediaProtocol *dpProtocol = (QEFIDevicePathMediaProtocol *)
                        dynamic_cast<QEFIDevicePathMediaProtocol *>(dp);
                    list << qMakePair<QString, enum QEFIDPEditType>("Protocol GUID",
                        QEFIDPEditType::EditType_UUID);
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_FirmwareFile:
                if (dynamic_cast<QEFIDevicePathMediaFirmwareFile *>(dp)) {
                    QEFIDevicePathMediaFirmwareFile *dpFile =
                        (QEFIDevicePathMediaFirmwareFile *)
                        dynamic_cast<QEFIDevicePathMediaFirmwareFile *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_FirmwareVolume:
                if (dynamic_cast<QEFIDevicePathMediaFile *>(dp)) {
                    QEFIDevicePathMediaFirmwareVolume *dpFV =
                        (QEFIDevicePathMediaFirmwareVolume *)
                        dynamic_cast<QEFIDevicePathMediaFirmwareVolume *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_RelativeOffset:
                if (dynamic_cast<QEFIDevicePathMediaRelativeOffset *>(dp)) {
                    QEFIDevicePathMediaRelativeOffset *dpRO =
                        (QEFIDevicePathMediaRelativeOffset *)
                        dynamic_cast<QEFIDevicePathMediaRelativeOffset *>(dp);
                    // TODO
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_RamDisk:
                if (dynamic_cast<QEFIDevicePathMediaRAMDisk *>(dp)) {
                    QEFIDevicePathMediaRAMDisk *dpRAMDisk =
                        (QEFIDevicePathMediaRAMDisk *)
                        dynamic_cast<QEFIDevicePathMediaRAMDisk *>(dp);
                    // TODO
                }
                break;
        }
        break;
    case QEFIDevicePathType::DP_BIOSBoot:
        if (dynamic_cast<QEFIDevicePathBIOSBoot *>(dp)) {
            QEFIDevicePathBIOSBoot *dpBIOSBoot = (QEFIDevicePathBIOSBoot *)
                dynamic_cast<QEFIDevicePathBIOSBoot *>(dp);
            // TODO
        }
        break;
    }
    return list;
}

QString convert_ipv4_to_string(const QEFIIPv4Address *ipv4) {
    const auto addr = ipv4->address;
    return QStringLiteral("%d.%d.%d.%d").arg(addr[0])
                                        .arg(addr[1])
                                        .arg(addr[2])
                                        .arg(addr[3]);
}

QList<quint8> enum_device_path_subtype(QEFIDevicePathType type)
{
    QList<quint8> res;
    switch (type)
    {
    case QEFIDevicePathType::DP_Hardware:
        // res << QEFIDevicePathHardwareSubType::HW_PCI
        //     << QEFIDevicePathHardwareSubType::HW_PCCard
        //     << QEFIDevicePathHardwareSubType::HW_MMIO
        //     << QEFIDevicePathHardwareSubType::HW_Vendor
        //     << QEFIDevicePathHardwareSubType::HW_Controller
        //     << QEFIDevicePathHardwareSubType::HW_BMC;
        break;
    case QEFIDevicePathType::DP_ACPI:
        // res << QEFIDevicePathACPISubType::ACPI_HID
        //     << QEFIDevicePathACPISubType::ACPI_HIDEX
        //     << QEFIDevicePathACPISubType::ACPI_ADR;
        break;
    case QEFIDevicePathType::DP_Message:
        // res << QEFIDevicePathMessageSubType::MSG_ATAPI
        //     << QEFIDevicePathMessageSubType::MSG_SCSI
        //     << QEFIDevicePathMessageSubType::MSG_FibreChan
        //     << QEFIDevicePathMessageSubType::MSG_1394
        //     << QEFIDevicePathMessageSubType::MSG_USB
        //     << QEFIDevicePathMessageSubType::MSG_I2O
        //     << QEFIDevicePathMessageSubType::MSG_InfiniBand
        //     << QEFIDevicePathMessageSubType::MSG_Vendor
        //     << QEFIDevicePathMessageSubType::MSG_MACAddr
        //     << QEFIDevicePathMessageSubType::MSG_IPv4
        //     << QEFIDevicePathMessageSubType::MSG_IPv6
        //     << QEFIDevicePathMessageSubType::MSG_UART
        //     << QEFIDevicePathMessageSubType::MSG_USBClass
        //     << QEFIDevicePathMessageSubType::MSG_USBWWID
        //     << QEFIDevicePathMessageSubType::MSG_LUN
        //     << QEFIDevicePathMessageSubType::MSG_SATA
        //     << QEFIDevicePathMessageSubType::MSG_ISCSI
        //     << QEFIDevicePathMessageSubType::MSG_VLAN
        //     << QEFIDevicePathMessageSubType::MSG_FibreChanEx
        //     << QEFIDevicePathMessageSubType::MSG_SASEX
        //     << QEFIDevicePathMessageSubType::MSG_NVME
        //     << QEFIDevicePathMessageSubType::MSG_URI
        //     << QEFIDevicePathMessageSubType::MSG_UFS
        //     << QEFIDevicePathMessageSubType::MSG_SD
        //     << QEFIDevicePathMessageSubType::MSG_BT
        //     << QEFIDevicePathMessageSubType::MSG_WiFi
        //     << QEFIDevicePathMessageSubType::MSG_EMMC
        //     << QEFIDevicePathMessageSubType::MSG_BTLE
        //     << QEFIDevicePathMessageSubType::MSG_DNS
        //     << QEFIDevicePathMessageSubType::MSG_NVDIMM;
        break;
    case QEFIDevicePathType::DP_Media:
        res << QEFIDevicePathMediaSubType::MEDIA_HD
            // << QEFIDevicePathMediaSubType::MEDIA_CDROM
            // << QEFIDevicePathMediaSubType::MEDIA_Vendor
            << QEFIDevicePathMediaSubType::MEDIA_File;
            // << QEFIDevicePathMediaSubType::MEDIA_Protocol
            // << QEFIDevicePathMediaSubType::MEDIA_FirmwareFile
            // << QEFIDevicePathMediaSubType::MEDIA_FirmwareVolume
            // << QEFIDevicePathMediaSubType::MEDIA_RelativeOffset
            // << QEFIDevicePathMediaSubType::MEDIA_RamDisk;
        break;
    default:
        break;
    }
    return res;
}
