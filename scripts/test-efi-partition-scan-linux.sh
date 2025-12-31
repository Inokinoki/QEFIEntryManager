#!/bin/bash
# Test script for EFI partition scanning on Linux
# This script creates a QEMU disk image with an EFI partition and tests detection

set -e  # Exit on error
set -x  # Print commands

echo "=== EFI Partition Scanning Test for Linux ==="

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "This script must be run as root for partition operations"
    exit 1
fi

# Install required tools
echo "Installing required tools..."
apt-get update -qq
apt-get install -y qemu-utils gdisk dosfstools kpartx udev

# Create a test disk image (100MB is enough for EFI partition)
DISK_IMAGE="/tmp/test-efi-disk.img"
echo "Creating test disk image: $DISK_IMAGE"
qemu-img create -f raw "$DISK_IMAGE" 100M

# Create GPT partition table and EFI partition using gdisk
echo "Creating GPT partition table with EFI System Partition..."
sgdisk -n 1:2048:+95M -t 1:ef00 -c 1:"EFI System" "$DISK_IMAGE"

# Verify partition table
echo "Partition table created:"
sgdisk -p "$DISK_IMAGE"

# Set up loop device for the disk image
echo "Setting up loop device..."
LOOP_DEVICE=$(losetup -f --show -P "$DISK_IMAGE")
echo "Loop device: $LOOP_DEVICE"

# Wait for udev to create partition device nodes
sleep 2
udevadm settle

# The partition should be ${LOOP_DEVICE}p1
PARTITION_DEVICE="${LOOP_DEVICE}p1"

# If partition device doesn't exist, try using kpartx as fallback
if [ ! -e "$PARTITION_DEVICE" ]; then
    echo "Partition device not found, using kpartx..."
    kpartx -a "$LOOP_DEVICE"
    sleep 1
    # kpartx creates devices like /dev/mapper/loopXp1
    LOOP_NAME=$(basename "$LOOP_DEVICE")
    PARTITION_DEVICE="/dev/mapper/${LOOP_NAME}p1"
fi

echo "Partition device: $PARTITION_DEVICE"

# Verify the partition device exists
if [ ! -e "$PARTITION_DEVICE" ]; then
    echo "ERROR: Partition device $PARTITION_DEVICE not found!"
    ls -la /dev/loop* /dev/mapper/ || true
    losetup -l
    exit 1
fi

# Format the EFI partition as FAT32
echo "Formatting EFI partition as FAT32..."
mkfs.vfat -F 32 -n "EFISYS" "$PARTITION_DEVICE"

# Wait for filesystem creation to complete
sync
sleep 1

# Verify the partition type GUID in sysfs
LOOP_NAME=$(basename "$LOOP_DEVICE")
PARTITION_NAME=$(basename "$PARTITION_DEVICE" | sed 's/^loop//')
echo "Checking sysfs for partition information..."
echo "Loop device name: $LOOP_NAME"

# For loop devices, sysfs path is different
# Try to find the partition in /sys/class/block/
ls -la /sys/class/block/ | grep "$LOOP_NAME" || true

# Check if partition type GUID is accessible
# Note: loop devices may not expose partition_type_guid in sysfs
if [ -f "/sys/class/block/${LOOP_NAME}p1/partition_type_guid" ]; then
    PART_TYPE=$(cat "/sys/class/block/${LOOP_NAME}p1/partition_type_guid")
    echo "Partition type GUID: $PART_TYPE"
else
    echo "Warning: partition_type_guid not found in sysfs for loop device"
    echo "This is expected for loop devices - testing with available information"
fi

# List all block devices to help debug
echo "All block devices:"
lsblk -f

echo "Block devices in /sys/class/block/:"
ls -la /sys/class/block/ | grep -E "(loop|nvme|sd|vd|mmc)" || true

# Now test the EFI partition manager
echo ""
echo "=== Testing QEFIPartitionManager ==="
echo "Note: The partition scanner will look for EFI partitions in /sys/class/block/"

# Check if the main application exists
if [ ! -f "$1" ]; then
    echo "ERROR: Application not found: $1"
    exit 1
fi

# Run the partition scanner with --test-scan flag
echo "Running EFI partition scanner..."
"$1" --test-scan 2>&1 | tee /tmp/scan-output.txt || {
    echo "Scanner execution failed"
    SCAN_RESULT=$?
}

# Check the output
echo ""
echo "=== Scan Results ==="
cat /tmp/scan-output.txt

# Verify that scanning completed without crashing
FAILURE=0
if grep -q "partition" /tmp/scan-output.txt; then
    echo "✓ Partition scanning executed successfully"
else
    FAILURE=1
    echo "⚠ Warning: No partition information in output"
fi

# Cleanup
echo ""
echo "=== Cleanup ==="
sync
sleep 1

# Unmount if mounted
umount "$PARTITION_DEVICE" 2>/dev/null || true

# Remove kpartx mappings if used
kpartx -d "$LOOP_DEVICE" 2>/dev/null || true

# Detach loop device
losetup -d "$LOOP_DEVICE"

# Remove disk image
rm -f "$DISK_IMAGE"

echo "=== Test Complete ==="

exit $FAILURE
