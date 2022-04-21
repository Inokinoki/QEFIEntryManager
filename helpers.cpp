#include "helpers.h"

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
                break;
            case QEFIDevicePathHardwareSubType::HW_PCCard:
                break;
            case QEFIDevicePathHardwareSubType::HW_MMIO:
                break;
            case QEFIDevicePathHardwareSubType::HW_Vendor:
                break;
            case QEFIDevicePathHardwareSubType::HW_Controller:
                break;
            case QEFIDevicePathHardwareSubType::HW_BMC:
                break;
        }
        break;
    case QEFIDevicePathType::DP_ACPI:
        switch (subtype) {
            case QEFIDevicePathACPISubType::ACPI_HID:
                break;
            case QEFIDevicePathACPISubType::ACPI_HIDEX:
                break;
            case QEFIDevicePathACPISubType::ACPI_ADR:
                break;
        }
        break;
    case QEFIDevicePathType::DP_Message:
        switch (subtype) {
            case QEFIDevicePathMessageSubType::MSG_ATAPI:
                break;
            case QEFIDevicePathMessageSubType::MSG_SCSI:
                break;
            case QEFIDevicePathMessageSubType::MSG_FibreChan:
                break;
            case QEFIDevicePathMessageSubType::MSG_1394:
                break;
            case QEFIDevicePathMessageSubType::MSG_USB:
                break;
            case QEFIDevicePathMessageSubType::MSG_I2O:
                break;
            case QEFIDevicePathMessageSubType::MSG_InfiniBand:
                break;
            case QEFIDevicePathMessageSubType::MSG_Vendor:
                break;
            case QEFIDevicePathMessageSubType::MSG_MACAddr:
                break;
            case QEFIDevicePathMessageSubType::MSG_IPv4:
                break;
            case QEFIDevicePathMessageSubType::MSG_IPv6:
                break;
            case QEFIDevicePathMessageSubType::MSG_UART:
                break;
            case QEFIDevicePathMessageSubType::MSG_USBClass:
                break;

            case QEFIDevicePathMessageSubType::MSG_USBWWID:
                break;

            case QEFIDevicePathMessageSubType::MSG_LUN:
                break;
            case QEFIDevicePathMessageSubType::MSG_SATA:
                break;
            case QEFIDevicePathMessageSubType::MSG_ISCSI:
                break;
            case QEFIDevicePathMessageSubType::MSG_VLAN:
                break;

            case QEFIDevicePathMessageSubType::MSG_FibreChanEx:
                break;
            case QEFIDevicePathMessageSubType::MSG_SASEX:
                break;

            case QEFIDevicePathMessageSubType::MSG_NVME:
                break;
            case QEFIDevicePathMessageSubType::MSG_URI:
                break;
            case QEFIDevicePathMessageSubType::MSG_UFS:
                break;
            case QEFIDevicePathMessageSubType::MSG_SD:
                break;
            case QEFIDevicePathMessageSubType::MSG_BT:
                break;
            case QEFIDevicePathMessageSubType::MSG_WiFi:
                break;
            case QEFIDevicePathMessageSubType::MSG_EMMC:
                break;
            case QEFIDevicePathMessageSubType::MSG_BTLE:
                break;
            case QEFIDevicePathMessageSubType::MSG_DNS:
                break;
            case QEFIDevicePathMessageSubType::MSG_NVDIMM:
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
                    list << qMakePair<QString, QString>("Data",
                        dpVendor->vendorData().size() < DISPLAY_DATA_LIMIT ?
                            dpVendor->vendorData().toHex() :
                            QString(dpVendor->vendorData()
                                .left(DISPLAY_DATA_LIMIT).toHex()) + "...");
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
                    list << qMakePair<QString, QString>("PI Info",
                        dpFile->piInfo().size() < DISPLAY_DATA_LIMIT ?
                            dpFile->piInfo().toHex() :
                            QString(dpFile->piInfo()
                                .left(DISPLAY_DATA_LIMIT).toHex()) + "...");
                }
                break;
            case QEFIDevicePathMediaSubType::MEDIA_FirmwareVolume:
                if (dynamic_cast<QEFIDevicePathMediaFile *>(dp)) {
                    QEFIDevicePathMediaFirmwareVolume *dpFV =
                        (QEFIDevicePathMediaFirmwareVolume *)
                        dynamic_cast<QEFIDevicePathMediaFirmwareVolume *>(dp);
                    list << qMakePair<QString, QString>("PI Info",
                        dpFV->piInfo().size() < DISPLAY_DATA_LIMIT ?
                            dpFV->piInfo().toHex() :
                            QString(dpFV->piInfo()
                                .left(DISPLAY_DATA_LIMIT).toHex()) + "...");
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
        break;
    }
    return list;
}
