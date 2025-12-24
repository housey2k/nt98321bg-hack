#!/bin/bash
set -e

echo "WARNING WARNING WARNING"
echo "This script requires you to go into the output directory and manually replace the partition at the right offset with a hex editor or dd"
echo "WARNING WARNING WARNING"

# Paths
SRC_DIR="squashfs-uncomp"
OUT_DIR="squashfs"

mkdir -p "$OUT_DIR"

# List of partitions to rebuild
PARTS=("usr" "web" "custom")  # Add/remove as needed

# Loop through each partition
for PART in "${PARTS[@]}"; do
    SRC="$SRC_DIR/$PART"
    OUT="$OUT_DIR/$PART.squashfs"

    if [ ! -d "$SRC" ]; then
        echo "Warning: source directory $SRC does not exist, skipping."
        continue
    fi

    echo "Building $OUT from $SRC..."
    mksquashfs "$SRC" "$OUT" -noappend -comp xz -b 131072 -Xbcj x86 -no-progress
done

echo "All squashfs images built successfully in $OUT_DIR."
