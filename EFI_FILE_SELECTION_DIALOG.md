# EFI File Selection Dialog

## Purpose

The EFI File Selection Dialog is specifically designed for **choosing EFI boot files from EFI partitions when creating new boot entries**. This feature addresses [Issue #13](https://github.com/Inokinoki/QEFIEntryManager/issues/13).

## Use Case

When creating a new boot entry in QEFIEntryManager, users need to specify:
1. Which EFI file to boot (e.g., `/EFI/BOOT/BOOTX64.EFI`)
2. Which partition contains the file
3. The device path structure for the UEFI firmware

Previously, users had to:
- Manually construct device paths
- Know the exact file path
- Understand partition offsets and structure

Now with the file selection dialog:
- Browse EFI partitions visually
- Navigate the FAT filesystem directory structure
- Select the EFI file with a click
- Automatically generate proper device paths

## User Workflow

### Creating a Boot Entry with File Selection

1. **Click "Add"** to create a new boot entry
2. **Click "Select from EFI Partition"** button in the editor
3. **Scan EFI Partitions** - Dialog automatically scans for all EFI system partitions
4. **Select Partition** - Choose from dropdown (shows size and label)
5. **Browse Filesystem** - Navigate directories to find your .efi file
6. **Select File** - Double-click or select and click OK
7. **Auto-Configuration** - Boot entry is automatically configured:
   - Entry name filled from filename
   - Hard Drive device path created with partition info
   - File Path device path created with file location
8. **Save Entry** - Complete the boot entry creation

## Technical Implementation

### Components

#### QEFIPartitionScanner
- **Purpose**: Low-level GPT partition table scanning
- **Features**:
  - Cross-platform (Linux, FreeBSD, Windows)
  - Direct disk access at byte offsets
  - GPT partition table parsing
  - EFI System Partition identification (GUID: C12A7328-F81F-11D2-BA4B-00A0C93EC93B)

#### QFATFilesystem (Submodule)
- **Repository**: [Inokinoki/QFATFilesystem](https://github.com/Inokinoki/QFATFilesystem)
- **Purpose**: Direct FAT12/16/32 filesystem reading
- **Features**:
  - Read FAT filesystem without mounting
  - Long filename (VFAT/LFN) support
  - Directory enumeration
  - File reading

#### QPartitionIODevice
- **Purpose**: QIODevice wrapper for partition access
- **Features**:
  - Wraps raw disk device
  - Handles partition offset translation
  - Provides standard QIODevice interface
  - Compatible with QFATFilesystem

#### QEFIFileSelectionDialog
- **Purpose**: UI for browsing EFI partition files
- **Features**:
  - Partition selection dropdown
  - Tree-based directory browser
  - File type filtering and icons
  - Size and type display
  - Parent directory navigation

### Integration Points

The dialog integrates into the boot entry creation workflow:

```cpp
// In QEFILoadOptionEditorView
void QEFILoadOptionEditorView::selectFromEFIClicked(bool checked)
{
    QEFIFileSelectionDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // Get selected file and partition info
        QString filePath = dialog.selectedFilePath();
        QEFIPartitionScanInfo partition = dialog.selectedPartition();

        // Automatically create device paths:
        // 1. Hard Drive Media Device Path (partition info)
        // 2. File Path Device Path (EFI file location)

        // Auto-fill boot entry name from filename
    }
}
```

## Platform Support

### Linux
- Scans `/sys/class/block` for GPT disks
- Device paths: `/dev/sda1`, `/dev/nvme0n1p1`, etc.
- Requires `sudo` or root privileges

### FreeBSD
- Scans `/dev` for disk devices (ada*, da*, nvd*)
- Device paths: `/dev/ada0p1`, `/dev/da0p1`, etc.
- Requires root privileges

### Windows
- Scans PhysicalDrive0-9 for GPT disks
- Device paths: `\\.\PhysicalDrive0`, etc.
- Requires "Run as Administrator"

## Advantages

### No Mounting Required
- Direct partition access at byte offsets
- Works even if partition is already mounted
- No temporary mount point creation
- Cleaner for one-time file selection

### Cross-Platform Consistency
- Same workflow on all platforms
- No OS-specific mount commands
- Unified UI and experience

### Boot Entry Automation
- Automatic device path generation
- Entry name auto-filled from filename
- Reduces user errors
- Faster workflow

### FAT Filesystem Support
- Full VFAT/LFN support for long filenames
- Handles FAT12, FAT16, and FAT32
- Standard for EFI System Partitions

## Limitations

### Administrator/Root Required
- Direct disk access requires elevated privileges
- Same requirement as mounting would have
- Necessary for reading partition tables

### FAT Filesystems Only
- EFI System Partitions are standardized as FAT
- This is not a limitation for the use case
- Other filesystems would require mounting (use PR #45)

### Read-Only Access
- Dialog only reads filesystem
- Cannot modify or create files
- Appropriate for boot entry creation workflow

## Comparison with PR #45

This feature is complementary to PR #45 (EFI Partition View):

| Feature | This Implementation | PR #45 |
|---------|---------------------|--------|
| **Purpose** | Boot entry creation | Partition management |
| **Access Method** | Direct FAT reading | OS mounting |
| **Use Case** | Select EFI file once | Manage partitions, access files |
| **Filesystem Support** | FAT only | Any OS-supported |
| **Mount Required** | No | Yes |
| **Persistent Access** | No (one-time selection) | Yes (files stay mounted) |

See [INTEGRATION_WITH_PR45.md](INTEGRATION_WITH_PR45.md) for detailed integration information.

## Future Enhancements

Potential improvements:
1. **Partition GUID extraction** - Auto-populate partition GUID in device path
2. **Partition number detection** - Extract actual partition number from GPT
3. **ACPI/PCI path generation** - Full device path chain from disk controller
4. **File validation** - Verify selected file is a valid EFI executable
5. **Recent files** - Remember recently selected EFI files
6. **Favorites** - Bookmark common EFI file locations

## Testing

The feature should be tested on:
- ✅ Linux (various distributions)
- ✅ Windows 10/11
- ✅ FreeBSD

Test scenarios:
- Multiple EFI partitions
- Various FAT filesystems (FAT12, FAT16, FAT32)
- Long filename support
- Deep directory structures
- Large partitions
- Permission handling (non-admin/root users)

## Credits

- Built for [QEFIEntryManager](https://github.com/Inokinoki/QEFIEntryManager) by Inokinoki
- Uses [QFATFilesystem](https://github.com/Inokinoki/QFATFilesystem) library
- Addresses [Issue #13](https://github.com/Inokinoki/QEFIEntryManager/issues/13)
- Developed in parallel with [PR #45](https://github.com/Inokinoki/QEFIEntryManager/pull/45)
