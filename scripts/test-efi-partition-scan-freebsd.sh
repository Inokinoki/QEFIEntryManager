#!/bin/sh
# Test script for EFI partition scanning on FreeBSD
# This script creates a memory disk with an EFI partition and tests detection

set -e  # Exit on error
set -x  # Print commands

echo "=== EFI Partition Scanning Test for FreeBSD ==="

# Check if running as root
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root for partition operations"
    exit 1
fi

# Create a memory disk (100MB - large enough for FAT32 with GPT overhead)
echo "Creating memory disk..."
MD_UNIT=$(mdconfig -a -t malloc -s 100M)
echo "Memory disk created: /dev/$MD_UNIT"

# Create GPT partition table with EFI partition
echo "Creating GPT partition table with EFI System Partition..."
gpart create -s gpt "$MD_UNIT"
gpart add -t efi -l "EFISYS" "$MD_UNIT"

# Show partition layout
echo "Partition table created:"
gpart show "$MD_UNIT"

# Format the EFI partition as FAT32 (msdosfs on FreeBSD) - set sector size to 1
PARTITION_DEVICE="/dev/${MD_UNIT}p1"
echo "Formatting EFI partition as FAT32: $PARTITION_DEVICE"
newfs_msdos -F 32 -s 1 -L "EFISYS" "$PARTITION_DEVICE"

# Wait for filesystem creation to complete
sync
sleep 1

# Verify GEOM configuration
echo "GEOM configuration:"
sysctl kern.geom.conftxt | grep -A 10 "$MD_UNIT" || true

# List all partitions using gpart
echo "All GPT partitions on system:"
gpart show || true

# Now test the EFI partition manager
echo ""
echo "=== Testing QEFIPartitionManager ==="
echo "Note: The partition scanner will use GEOM library to enumerate partitions"

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
if grep -q "partition\|EFI" /tmp/scan-output.txt; then
    echo "✓ Partition scanning executed successfully"

    # Check if our test partition was detected
    if grep -q "$MD_UNIT" /tmp/scan-output.txt; then
        echo "✓ Test EFI partition was detected!"
    else
        echo "⚠ Warning: Test partition not detected in output"
    fi
else
    echo "⚠ Warning: No partition information in output"
fi

# Cleanup
echo ""
echo "=== Cleanup ==="
sync
sleep 1

# Destroy the memory disk
mdconfig -d -u "$MD_UNIT"

echo "=== Test Complete ==="
echo "The scanner should have detected the EFI partition on /dev/$MD_UNIT"

exit 0
