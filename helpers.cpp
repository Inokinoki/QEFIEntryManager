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
