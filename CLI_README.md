# QEFIEntryManager CLI Mode

QEFIEntryManager now supports a command-line interface (CLI) mode that is compatible with `efibootmgr` command-line options. This allows you to manage EFI boot entries directly from the terminal without launching the GUI.

## CLI Executables

The project provides three ways to use the CLI:

1. **qefibootmgr** - Dedicated CLI-only executable (recommended for CLI usage)
2. **efibootmgr** - Drop-in compatible alias for standard efibootmgr
   - On Unix-like systems (Linux, macOS, FreeBSD): Symlink to qefibootmgr
   - On Windows: Separate executable name (same binary as qefibootmgr)
3. **QEFIEntryManager** - Main executable that auto-detects CLI mode when arguments are provided

## Usage

### Using qefibootmgr or efibootmgr

```bash
# Using qefibootmgr (dedicated CLI tool)
sudo qefibootmgr -v
sudo qefibootmgr --help

# Using efibootmgr alias (drop-in replacement)
sudo efibootmgr -v
sudo efibootmgr -o 0001,0000,0002
```

### Using QEFIEntryManager

The main application automatically detects CLI mode when you pass command-line arguments. Without arguments, it launches the GUI.

```bash
# Launch GUI (no arguments)
sudo QEFIEntryManager

# Launch CLI (with any argument)
sudo QEFIEntryManager --help
sudo QEFIEntryManager -v
```

## Requirements

- **Administrator/Root privileges**: Required for modifying EFI variables
  - **Windows**: Run as Administrator (the executables include a manifest for UAC elevation)
  - **Linux/macOS**: Use `sudo` or run as root
  - **FreeBSD**: Use `su` or `doas`
- **EFI environment**: System must be booted in UEFI mode (not BIOS/CSM)

## Command-Line Options

### Display Options

- `-h, --help` - Display help information
- `-v, --verbose` - Verbose mode - prints additional information including device paths
- `-q, --quiet` - Quiet mode - suppresses output
- `-V, --version` - Print version string and exit

### Viewing Boot Entries

By default (or after modifications), the CLI displays:
- Timeout value
- Boot order
- Current boot entry (BootCurrent)
- Next boot entry (BootNext, if set)
- All boot entries with their IDs, names, and active status

```bash
# Display boot entries
sudo QEFIEntryManager -v

# Example output:
# Timeout: 5 seconds
# BootOrder: 0000,0001,0002
# BootCurrent: 0000
# Boot0000* Ubuntu        HD(1,GPT,...)/File(\EFI\ubuntu\shimx64.efi)
# Boot0001  Windows Boot Manager    HD(2,GPT,...)/File(\EFI\Microsoft\Boot\bootmgfw.efi)
# Boot0002* UEFI OS       HD(1,GPT,...)
```

### Boot Entry Selection

- `-b, --bootnum XXXX` - Specify boot entry number in hex (e.g., `0000`, `0001`)

### Modifying Boot Entries

- `-a, --active` - Set bootnum active (requires `-b`)
- `-A, --inactive` - Set bootnum inactive (requires `-b`)
- `-B, --delete-bootnum` - Delete bootnum (requires `-b`)

```bash
# Set Boot0001 as active
sudo QEFIEntryManager -b 0001 -a

# Set Boot0002 as inactive
sudo QEFIEntryManager -b 0002 -A

# Delete Boot0003
sudo QEFIEntryManager -b 0003 -B
```

### Boot Order Management

- `-o, --bootorder XXXX,YYYY,ZZZZ` - Explicitly set boot order (hex, comma-separated)
- `-O, --delete-bootorder` - Delete boot order
- `-n, --bootnext XXXX` - Set BootNext to XXXX (hex) - boots this entry once on next reboot
- `-N, --delete-bootnext` - Delete BootNext

```bash
# Set boot order
sudo QEFIEntryManager -o 0001,0000,0002

# Set next boot to Boot0001 (one-time boot)
sudo QEFIEntryManager -n 0001

# Delete BootNext
sudo QEFIEntryManager -N
```

### Timeout Management

- `-t, --timeout SECONDS` - Set boot manager timeout in seconds
- `-T, --delete-timeout` - Delete timeout variable

```bash
# Set timeout to 10 seconds
sudo QEFIEntryManager -t 10

# Delete timeout
sudo QEFIEntryManager -T
```

### Maintenance

- `-D, --remove-dups` - Remove duplicate entries from boot order

```bash
# Remove duplicates from boot order
sudo QEFIEntryManager -D
```

### Boot Entry Creation

- `-c, --create` - Create new boot entry and add to boot order
- `-C, --create-only` - Create new boot entry without adding to boot order
- `-d, --disk DISK` - Disk containing the loader (e.g., `/dev/sda`)
- `-p, --part PART` - Partition number (e.g., `1`)
- `-l, --loader NAME` - Loader path (e.g., `\EFI\ubuntu\grubx64.efi`)
- `-L, --label LABEL` - Boot entry label (e.g., `"My Linux"`)

**Note**: Boot entry creation is not yet fully implemented in CLI mode. Please use the GUI for creating new boot entries.

## Examples

All examples below can use `qefibootmgr`, `efibootmgr`, or `QEFIEntryManager` interchangeably.

### Example 1: View all boot entries with verbose output
```bash
sudo qefibootmgr -v
# or
sudo efibootmgr -v
```

### Example 2: Change boot order to prioritize Windows
```bash
sudo efibootmgr -o 0001,0000,0002
```

### Example 3: Disable a boot entry
```bash
sudo qefibootmgr -b 0002 -A
```

### Example 4: Boot into Windows on next reboot only
```bash
sudo efibootmgr -n 0001 && sudo reboot
```

### Example 5: Set timeout and clean up duplicates
```bash
sudo qefibootmgr -t 3 -D
```

### Example 6: Drop-in replacement for efibootmgr scripts
```bash
# This works with existing efibootmgr scripts!
sudo efibootmgr -v | grep "Boot0000"
sudo efibootmgr -b 0001 -a
```

## Compatibility with efibootmgr

This CLI mode is designed to be compatible with the standard `efibootmgr` command-line tool. Most common options are supported with the same syntax:

- Display options: `-v`, `-q`, `-V`
- Boot entry selection: `-b`
- Modifications: `-a`, `-A`, `-B`
- Boot order: `-o`, `-O`
- BootNext: `-n`, `-N`
- Timeout: `-t`, `-T`
- Maintenance: `-D`

## Differences from efibootmgr

Some advanced options are not yet implemented:
- Boot entry creation (`-c`, `-C`) with full device path specification
- Network boot options (`-i`)
- EDD options (`-e`, `-E`)
- Driver variables (`-r`)
- SysPrep variables (`-y`)
- Binary arguments (`-@`)
- Mirror options (`-m`, `-M`)

For these advanced features, please use the GUI mode or contribute to the project!

## Windows-Specific Notes

On Windows, the CLI executables (`qefibootmgr.exe` and `efibootmgr.exe`) are built with an embedded manifest that requests administrator elevation:

### Running on Windows:

```cmd
# The UAC prompt will appear automatically when you run the executable
qefibootmgr -v
efibootmgr -v

# Or run from an elevated Command Prompt or PowerShell
# Right-click Command Prompt/PowerShell -> "Run as Administrator"
qefibootmgr -o 0001,0000,0002
```

### Important Windows Considerations:

- Both `qefibootmgr.exe` and `efibootmgr.exe` include UAC manifests for automatic elevation
- You cannot use these from a normal command prompt without the UAC dialog appearing
- For scripting, run your script from an already-elevated console
- On Windows, use backslashes in paths: `\EFI\Microsoft\Boot\bootmgfw.efi`
- Output uses Windows line endings (CRLF)

## Exit Codes

- `0` - Success
- `1` - Error (no EFI environment, insufficient privileges, invalid arguments, or operation failed)

## Notes

- All boot entry numbers are specified in hexadecimal (e.g., `0000`, `0001`, `00FF`)
- Changes are immediate and permanent - use with caution
- Always verify your changes with `-v` before rebooting
- Keep a backup boot entry or recovery media available

## Troubleshooting

### "No EFI environment available"
Your system is not booted in UEFI mode. Reboot and ensure you're using UEFI boot, not legacy BIOS.

### "Insufficient privileges"
You need to run the command with `sudo` (Linux/macOS) or as Administrator (Windows).

### Invalid bootnum format
Boot entry numbers must be in hexadecimal format (e.g., `0000`, `0001`, not `0`, `1`).

## Contributing

This CLI mode is part of the QEFIEntryManager project. Contributions are welcome!

Repository: https://github.com/Inokinoki/QEFIEntryManager
