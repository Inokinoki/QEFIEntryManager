# QEFI Entry Manager

[![Linux x64 AppImage](https://github.com/Inokinoki/QEFIEntryManager/actions/workflows/cmake-linux-amd64-appimage.yml/badge.svg)](https://github.com/Inokinoki/QEFIEntryManager/actions/workflows/cmake-linux-amd64-appimage.yml)

An EFI manager in Qt.

## Usage

This application needs to be run with root/sudo on Linux, or `Run as administrator` on Windows.

You can change the boot order:

![Boot Entry](.github/entries.png)

or temporally set the next boot entry in this app:

![Reboot](.github/reboot.png)

click on `Yes` to reboot immediately:

![Reboot Confirmation](.github/reboot_confirm.png)

## Install

Download the prebuilt app from CI:

- [Linux AppImages from CI](https://github.com/Inokinoki/QEFIEntryManager/actions/runs)
- TODO: [Windows]()

Run it as root (*nix) or administrator (Windows).

## Build from scratch

Clone and compile this project:

```
git clone --recursive https://github.com/Inokinoki/QEFIEntryManager.git
cd QEFIEntryManager
mkdir build && cd build
cmake ..
make
```

And there will be an executable `QEFIEntryManager` in your build directory. Run it as root (*nix) or administrator (Windows).
