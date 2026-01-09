#!/bin/bash
# Script to generate a test EFI disk image with FAT32 partition
# Compatible with macOS and Linux
# For testing EFI entry scanning and creation in QEFientryManager

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${SCRIPT_DIR}/../test_data"
IMAGE_SIZE_MB=64
IMAGE_FILE="test_efi_disk.img"
IMAGE_PATH="${OUTPUT_DIR}/${IMAGE_FILE}"
TEMP_DIR="temp_efi_files"
MOUNT_POINT="temp_efi_mount"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

echo_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

echo_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Detect OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    IS_MACOS=1
    USE_MTOOLS=0
else
    IS_MACOS=0
    # Check if mtools is available
    if command -v mcopy &> /dev/null; then
        USE_MTOOLS=1
    else
        USE_MTOOLS=0
    fi
fi

# Check for required commands
check_requirements() {
    local missing=0

    if [[ $USE_MTOOLS -eq 1 ]]; then
        if ! command -v mkfs.fat &> /dev/null; then
            echo_error "mkfs.fat not found. Install dosfstools package."
            missing=1
        fi
        if ! command -v mcopy &> /dev/null; then
            echo_error "mtools not found. Install mtools package."
            missing=1
        fi
    elif [[ $IS_MACOS -eq 1 ]]; then
        if ! command -v hdiutil &> /dev/null; then
            echo_error "hdiutil not found (required on macOS)"
            missing=1
        fi
        if ! command -v newfs_msdos &> /dev/null; then
            echo_error "newfs_msdos not found (required on macOS)"
            missing=1
        fi
    else
        if ! command -v mkfs.fat &> /dev/null; then
            echo_error "mkfs.fat not found. Install dosfstools package."
            missing=1
        fi
        if ! command -v mount &> /dev/null; then
            echo_error "mount command not found"
            missing=1
        fi
    fi

    if ! command -v dd &> /dev/null; then
        echo_error "dd not found"
        missing=1
    fi

    if [[ $missing -eq 1 ]]; then
        echo_error "Missing required tools. Exiting."
        exit 1
    fi
}

# Create output directory
mkdir -p "${OUTPUT_DIR}"

# Function to create and mount a FAT32 image
create_fat32_image() {
    local IMAGE_PATH=$1
    local SIZE_MB=$2
    local MOUNT_POINT=$3

    echo_info "Creating FAT32 image: ${IMAGE_PATH} (${SIZE_MB}MB)"

    # Create empty image file
    dd if=/dev/zero of="${IMAGE_PATH}" bs=1M count=${SIZE_MB} 2>/dev/null || {
        echo_error "Failed to create image file"
        exit 1
    }

    if [[ $IS_MACOS -eq 1 ]]; then
        # macOS-specific approach using hdiutil
        hdiutil attach -nomount "${IMAGE_PATH}" > /tmp/hdiutil_output.txt
        local DISK_DEV=$(cat /tmp/hdiutil_output.txt | awk '{print $1}')
        newfs_msdos -F 32 -v "EFI TEST" "${DISK_DEV}"
        hdiutil detach "${DISK_DEV}"
        rm -f /tmp/hdiutil_output.txt

        # Mount the image
        mkdir -p "${MOUNT_POINT}"
        hdiutil attach "${IMAGE_PATH}" -mountpoint "${MOUNT_POINT}"
    else
        # Linux-specific approach
        mkfs.fat -F 32 -s 2 -S 512 -n "EFI TEST" "${IMAGE_PATH}" || {
            echo_error "Failed to create FAT32 filesystem"
            exit 1
        }

        # Mount the image
        mkdir -p "${MOUNT_POINT}"
        # Mount with uid and gid to allow current user to write
        sudo mount -o loop,uid=$(id -u),gid=$(id -g) "${IMAGE_PATH}" "${MOUNT_POINT}" || {
            echo_error "Failed to mount ${IMAGE_PATH}"
            exit 1
        }
    fi

    echo_info "Image mounted at ${MOUNT_POINT}"
}

# Function to create FAT32 image without mounting (using mtools)
create_fat32_image_mtools() {
    local IMAGE_PATH=$1
    local SIZE_MB=$2

    echo_info "Creating FAT32 image using mtools: ${IMAGE_PATH} (${SIZE_MB}MB)"

    # Create empty image file
    dd if=/dev/zero of="${IMAGE_PATH}" bs=1M count=${SIZE_MB} 2>/dev/null || {
        echo_error "Failed to create image file"
        exit 1
    }

    # Format the image
    mkfs.fat -F 32 -s 2 -S 512 -n "EFI TEST" "${IMAGE_PATH}" > /dev/null 2>&1 || {
        echo_error "Failed to create FAT32 filesystem"
        exit 1
    }

    echo_info "FAT32 filesystem created"
}

# Function to populate image with EFI test content
populate_efi_test_content() {
    local MOUNT_POINT=$1

    echo_info "Populating EFI test content at ${MOUNT_POINT}..."

    # Verify mount point is accessible
    if [[ ! -d "${MOUNT_POINT}" ]]; then
        echo_error "Mount point ${MOUNT_POINT} is not accessible"
        return 1
    fi

    # Create EFI directory structure
    mkdir -p "${MOUNT_POINT}/EFI/BOOT"
    mkdir -p "${MOUNT_POINT}/EFI/test_bootloader"
    mkdir -p "${MOUNT_POINT}/EFI/grub"
    mkdir -p "${MOUNT_POINT}/EFI/systemd"
    mkdir -p "${MOUNT_POINT}/EFI/Microsoft/Boot"
    mkdir -p "${MOUNT_POINT}/loader/entries"

    # Create test EFI files (placeholders)
    # 1. EFI Boot Manager placeholder
    cat > "${MOUNT_POINT}/EFI/BOOT/BOOTX64.EFI" << 'EOF'
This is a placeholder for BOOTX64.EFI - EFI Boot Manager
In a real EFI system, this would be a binary EFI application.
EOF

    # 2. Test bootloader
    cat > "${MOUNT_POINT}/EFI/test_bootloader/bootx64.efi" << 'EOF'
This is a placeholder for test_bootloader.efi
EOF

    # 3. GRUB files
    cat > "${MOUNT_POINT}/EFI/grub/grubx64.efi" << 'EOF'
This is a placeholder for grubx64.efi
EOF

    cat > "${MOUNT_POINT}/EFI/grub/grub.cfg" << 'EOF'
# GRUB Configuration File
set timeout=5
set default=0

menuentry "Test Operating System 1" {
    chainloader /EFI/test_bootloader/bootx64.efi
}

menuentry "Test Operating System 2" {
    chainloader /EFI/BOOT/BOOTX64.EFI
}

menuentry "Recovery Mode" {
    chainloader /EFI/BOOT/BOOTX64.EFI
}
EOF

    # 4. systemd-boot files
    cat > "${MOUNT_POINT}/EFI/systemd/systemd-bootx64.efi" << 'EOF'
This is a placeholder for systemd-bootx64.efi
EOF

    cat > "${MOUNT_POINT}/EFI/systemd/loader.conf" << 'EOF'
# systemd-boot configuration
timeout 5
default test_os1
editor 1
auto-entries 1
auto-firmware 1
EOF

    # 5. systemd-boot loader entries
    cat > "${MOUNT_POINT}/loader/entries/test_os1.conf" << 'EOF'
title Test Operating System 1
efi /EFI/test_bootloader/bootx64.efi
options quiet splash
EOF

    cat > "${MOUNT_POINT}/loader/entries/test_os2.conf" << 'EOF'
title Test Operating System 2
efi /EFI/BOOT/BOOTX64.EFI
options quiet
EOF

    cat > "${MOUNT_POINT}/loader/entries/recovery.conf" << 'EOF'
title Recovery Mode
efi /EFI/test_bootloader/bootx64.efi
options single
EOF

    # 6. Windows Boot Manager placeholder
    cat > "${MOUNT_POINT}/EFI/Microsoft/Boot/bootmgfw.efi" << 'EOF'
This is a placeholder for Windows Boot Manager (bootmgfw.efi)
EOF

    # Sync to ensure all data is written to disk
    sync

    # Give the filesystem time to flush on Linux
    if [[ $IS_MACOS -eq 0 ]]; then
        sleep 1
    fi

    echo_info "EFI test content created successfully"
}

# Function to populate image with EFI test content using mtools (no mounting required)
populate_efi_test_content_mtools() {
    local IMAGE_PATH=$1
    local TEMP_DIR=$(mktemp -d)

    echo_info "Populating EFI test content using mtools (no mount required)..."

    # Create EFI directory structure in temp directory
    mkdir -p "${TEMP_DIR}/EFI/BOOT"
    mkdir -p "${TEMP_DIR}/EFI/test_bootloader"
    mkdir -p "${TEMP_DIR}/EFI/grub"
    mkdir -p "${TEMP_DIR}/EFI/systemd"
    mkdir -p "${TEMP_DIR}/EFI/Microsoft/Boot"
    mkdir -p "${TEMP_DIR}/loader/entries"

    # Create test EFI files (placeholders)
    # 1. EFI Boot Manager placeholder
    cat > "${TEMP_DIR}/EFI/BOOT/BOOTX64.EFI" << 'EOF'
This is a placeholder for BOOTX64.EFI - EFI Boot Manager
In a real EFI system, this would be a binary EFI application.
EOF

    # 2. Test bootloader
    cat > "${TEMP_DIR}/EFI/test_bootloader/bootx64.efi" << 'EOF'
This is a placeholder for test_bootloader.efi
EOF

    # 3. GRUB files
    cat > "${TEMP_DIR}/EFI/grub/grubx64.efi" << 'EOF'
This is a placeholder for grubx64.efi
EOF

    cat > "${TEMP_DIR}/EFI/grub/grub.cfg" << 'EOF'
# GRUB Configuration File
set timeout=5
set default=0

menuentry "Test Operating System 1" {
    chainloader /EFI/test_bootloader/bootx64.efi
}

menuentry "Test Operating System 2" {
    chainloader /EFI/BOOT/BOOTX64.EFI
}

menuentry "Recovery Mode" {
    chainloader /EFI/BOOT/BOOTX64.EFI
}
EOF

    # 4. systemd-boot files
    cat > "${TEMP_DIR}/EFI/systemd/systemd-bootx64.efi" << 'EOF'
This is a placeholder for systemd-bootx64.efi
EOF

    cat > "${TEMP_DIR}/EFI/systemd/loader.conf" << 'EOF'
# systemd-boot configuration
timeout 5
default test_os1
editor 1
auto-entries 1
auto-firmware 1
EOF

    # 5. systemd-boot loader entries
    cat > "${TEMP_DIR}/loader/entries/test_os1.conf" << 'EOF'
title Test Operating System 1
efi /EFI/test_bootloader/bootx64.efi
options quiet splash
EOF

    cat > "${TEMP_DIR}/loader/entries/test_os2.conf" << 'EOF'
title Test Operating System 2
efi /EFI/BOOT/BOOTX64.EFI
options quiet
EOF

    cat > "${TEMP_DIR}/loader/entries/recovery.conf" << 'EOF'
title Recovery Mode
efi /EFI/test_bootloader/bootx64.efi
options single
EOF

    # 6. Windows Boot Manager placeholder
    cat > "${TEMP_DIR}/EFI/Microsoft/Boot/bootmgfw.efi" << 'EOF'
This is a placeholder for Windows Boot Manager (bootmgfw.efi)
EOF

    # Use mtools to copy files into the image (no mounting required!)
    export MTOOLS_SKIP_CHECK=1

    # Create directories
    mmd -i "${IMAGE_PATH}" ::/EFI 2>/dev/null
    mmd -i "${IMAGE_PATH}" ::/EFI/BOOT 2>/dev/null
    mmd -i "${IMAGE_PATH}" ::/EFI/test_bootloader 2>/dev/null
    mmd -i "${IMAGE_PATH}" ::/EFI/grub 2>/dev/null
    mmd -i "${IMAGE_PATH}" ::/EFI/systemd 2>/dev/null
    mmd -i "${IMAGE_PATH}" ::/EFI/Microsoft 2>/dev/null
    mmd -i "${IMAGE_PATH}" ::/EFI/Microsoft/Boot 2>/dev/null
    mmd -i "${IMAGE_PATH}" ::/loader 2>/dev/null
    mmd -i "${IMAGE_PATH}" ::/loader/entries 2>/dev/null

    # Copy files
    mcopy -i "${IMAGE_PATH}" "${TEMP_DIR}/EFI/BOOT/BOOTX64.EFI" ::/EFI/BOOT/ 2>/dev/null
    mcopy -i "${IMAGE_PATH}" "${TEMP_DIR}/EFI/test_bootloader/bootx64.efi" ::/EFI/test_bootloader/ 2>/dev/null
    mcopy -i "${IMAGE_PATH}" "${TEMP_DIR}/EFI/grub/grubx64.efi" ::/EFI/grub/ 2>/dev/null
    mcopy -i "${IMAGE_PATH}" "${TEMP_DIR}/EFI/grub/grub.cfg" ::/EFI/grub/ 2>/dev/null
    mcopy -i "${IMAGE_PATH}" "${TEMP_DIR}/EFI/systemd/systemd-bootx64.efi" ::/EFI/systemd/ 2>/dev/null
    mcopy -i "${IMAGE_PATH}" "${TEMP_DIR}/EFI/systemd/loader.conf" ::/EFI/systemd/ 2>/dev/null
    mcopy -i "${IMAGE_PATH}" "${TEMP_DIR}/loader/entries/test_os1.conf" ::/loader/entries/ 2>/dev/null
    mcopy -i "${IMAGE_PATH}" "${TEMP_DIR}/loader/entries/test_os2.conf" ::/loader/entries/ 2>/dev/null
    mcopy -i "${IMAGE_PATH}" "${TEMP_DIR}/loader/entries/recovery.conf" ::/loader/entries/ 2>/dev/null
    mcopy -i "${IMAGE_PATH}" "${TEMP_DIR}/EFI/Microsoft/Boot/bootmgfw.efi" ::/EFI/Microsoft/Boot/ 2>/dev/null

    # Clean up temp directory
    rm -rf "${TEMP_DIR}"

    echo_info "EFI test content created successfully"
}

# Function to unmount and cleanup
unmount_image() {
    local MOUNT_POINT=$1

    if [[ $IS_MACOS -eq 1 ]]; then
        hdiutil detach "${MOUNT_POINT}" 2>/dev/null || true
    else
        sudo umount "${MOUNT_POINT}" 2>/dev/null || true
    fi
    rmdir "${MOUNT_POINT}" 2>/dev/null || true
}

# Main execution
echo_info "Generating FAT32 EFI test image for QEFientryManager testing..."
check_requirements

# Clean up any existing mount points (only needed for mount-based approach)
if [[ $USE_MTOOLS -eq 0 ]]; then
    unmount_image "/tmp/efi_mount" 2>/dev/null || true
fi

# Remove existing image if present
if [[ -f "${IMAGE_PATH}" ]]; then
    echo_warn "Removing existing EFI test image"
    rm -f "${IMAGE_PATH}"
fi

# Generate FAT32 image with EFI content
EFI_MOUNT="/tmp/efi_mount"

if [[ $USE_MTOOLS -eq 1 ]]; then
    # Use mtools approach (no mounting)
    create_fat32_image_mtools "${IMAGE_PATH}" ${IMAGE_SIZE_MB}
    populate_efi_test_content_mtools "${IMAGE_PATH}" || {
        echo_error "Failed to populate EFI test image"
        exit 1
    }
else
    # Use traditional mount approach
    create_fat32_image "${IMAGE_PATH}" ${IMAGE_SIZE_MB} "${EFI_MOUNT}"
    populate_efi_test_content "${EFI_MOUNT}" || {
        echo_error "Failed to populate EFI test image"
        unmount_image "${EFI_MOUNT}"
        exit 1
    }
    unmount_image "${EFI_MOUNT}"
fi

echo_info "FAT32 EFI test image created successfully: ${IMAGE_PATH}"

# Create compressed version
echo_info "Creating compressed version..."
gzip -c "${IMAGE_PATH}" > "${IMAGE_PATH}.gz"

echo ""
echo_info "========================================="
echo_info "✓ Test EFI disk image created successfully!"
echo_info "========================================="
echo ""
echo_info "Files created:"
echo_info "  - ${IMAGE_PATH} (uncompressed)"
echo_info "  - ${IMAGE_PATH}.gz (compressed)"
echo ""
echo_info "Disk image size: ${IMAGE_SIZE_MB}MB"
echo_info "Filesystem: FAT32"
echo ""
echo_info "EFI Structure created:"
echo_info "  /EFI/BOOT/BOOTX64.EFI - EFI Boot Manager"
echo_info "  /EFI/test_bootloader/bootx64.efi - Test bootloader"
echo_info "  /EFI/grub/grubx64.efi - GRUB bootloader"
echo_info "  /EFI/grub/grub.cfg - GRUB configuration"
echo_info "  /EFI/systemd/systemd-bootx64.efi - systemd-boot"
echo_info "  /EFI/systemd/loader.conf - systemd-boot config"
echo_info "  /loader/entries/*.conf - Boot entries"
echo_info "  /EFI/Microsoft/Boot/bootmgfw.efi - Windows Boot Manager"
echo ""
echo_info "To mount and inspect (macOS):"
echo_info "  hdiutil attach ${IMAGE_PATH}"
echo_info "  ls -la /Volumes/EFI TEST"
echo_info "  hdiutil detach /Volumes/EFI TEST"
echo ""
echo_info "To mount and inspect (Linux):"
echo_info "  sudo mount -o loop ${IMAGE_PATH} /mnt/efi"
echo_info "  ls -la /mnt/efi"
echo_info "  sudo umount /mnt/efi"
echo ""
echo_info "To use with QEFientryManager for testing:"
echo_info "  1. Mount the image:"
echo_info "     macOS: hdiutil attach ${IMAGE_PATH}"
echo_info "     Linux: sudo mount -o loop ${IMAGE_PATH} /mnt/efi"
echo ""
echo_info "  2. Use the mounted path in QEFientryManager to:"
echo_info "     - Scan for EFI files"
echo_info "     - Create boot entries from mounted EFI files"
echo_info "     - Test entry creation from unsupported platforms"
echo ""
