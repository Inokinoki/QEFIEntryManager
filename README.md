# QEFI Entry Manager

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

First, compile and install the [QEFI]() dependency:

```
git clone https://github.com/Inokinoki/qefivar.git
cd qefivar
mkdir build && cd build
cmake ..
make install
```

Then, clone and compile this project:

```
git clone https://github.com/Inokinoki/QEFIEntryManager.git
cd QEFIEntryManager
mkdir build && cd build
cmake ..
make
```

And there will be an executable `QEFIEntryManager` in your build directory. Run it as root (*nix) or administrator (Windows).
