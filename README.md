# QEFI Entry Manager

[![Linux x64 AppImage](https://github.com/Inokinoki/QEFIEntryManager/actions/workflows/cmake-linux-amd64-appimage.yml/badge.svg)](https://github.com/Inokinoki/QEFIEntryManager/actions/workflows/cmake-linux-amd64-appimage.yml)

[![Build Windows x64](https://github.com/Inokinoki/QEFIEntryManager/actions/workflows/cmake-windows-x86-x64.yml/badge.svg)](https://github.com/Inokinoki/QEFIEntryManager/actions/workflows/cmake-windows-x86-x64.yml)

[![Build on FreeBSD](https://github.com/Inokinoki/QEFIEntryManager/actions/workflows/cmake-freebsd-amd64.yml/badge.svg)](https://github.com/Inokinoki/QEFIEntryManager/actions/workflows/cmake-freebsd-amd64.yml)

An EFI entry and partition manager in Qt.

## Usage

This application needs to be run with root/sudo on Linux, or `Run as administrator` on Windows.

### Boot Entry Management

You can **change the boot order, add/import new boot entry**:

![Boot Entry](.github/main.png)

or temporally set the next boot entry to **quickly reboot to another OS**.

After setting, click on `Yes` to reboot immediately:

![Reboot Confirmation](.github/reboot_confirm.png)

Otherwise, it will boot to the other OS once after your manual reboot.

Right click on the boot entry to **enable/disable/delete the entry**, or **show the detailed properties**.

### Partition Management

You can also manage the EFI system partitions on your disks:

![Partition Management](.github/partitions.png)

Choose the partition to **mount/unmount the partition**, or **open the partition in file manager** (on Windows, it will be a file dialog due to system limit). You can view or modify (move, rename, copy, etc.) the files in the parition.

You can also **create a new EFI entry from the partition** by clicking "Create" button. Select the **EFI application file** (usually with `.efi` extension) in the partition, and fill in the other information (simply the boot entry number and the name are enough in most cases):

![Create Entry](.github/partition_efi_entry.png)

**Note that this is a beta feature, and may not work on all systems. Use it with caution.**

## Install

### Arch Linux

#### AUR

- Stable version:

   ```shell
   [yay/paru] -S qefientrymanager
   ```

- Latest git version:

   ```shell
   [yay/paru] -S qefientrymanager-git
   ```

#### `archlinuxcn`

- Stable version:

   ```shell
   sudo pacman -S qefientrymanager
   ```

- Latest git version:

   ```shell
   sudo pacman -S qefientrymanager-git
   ```

### Other Linux Distros & Windows

Download the prebuilt app from [release](https://github.com/Inokinoki/QEFIEntryManager/releases) page.

Run it as root (*nix):

```shell
sudo -E ./<executable>
```

or administrator on Windows(should be automatic with UAC, otherwise right click it).

## Build from scratch

For older versions (such as Ubuntu 16, 18, 20, etc.) or other architectures (e.g., ARM, aarch64), pre-built binaries are not provided.

Clone and compile this project:

```shell
git clone --recursive https://github.com/Inokinoki/QEFIEntryManager.git
cd QEFIEntryManager
mkdir build && cd build
cmake ..
make
```

And there will be an executable `QEFIEntryManager` in your build directory. Run it as root (*nix) or administrator (Windows).
