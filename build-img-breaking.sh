#!/bin/sh
set -e

echo "WARNING WARNING WARNING"
echo "This script has the convenience of making a .bin file directly, but it risks breaking stuff, one of them is the U-Boot logo splash that is replaced by a test pattern"
echo "WARNING WARNING WARNING"

OUT=firmware-rebuilt.bin
gcd=16

TMP_SQ=tmp-squashfs
FLASH_SIZE=$((0x1000000))   # 16 MB
FLASH_BLOCKS=$((FLASH_SIZE / gcd))

mkdir -p "$TMP_SQ"

echo "[*] Rebuilding SquashFS images..."

# Helper: rebuild squashfs using original image as reference
rebuild_sq() {
    name="$1"
    orig="work/${name}.squashfs"
    src="squashfs-uncomp/${name}"
    out="$TMP_SQ/${name}.squashfs"

    echo "  → $name"

    info=$(file "$orig")

    blocksize=$(echo "$info" | sed -n 's/.*blocksize: \([0-9]*\).*/\1/p')
    comp=$(echo "$info" | grep -o "xz compressed\|gzip compressed\|lzo compressed\|lz4 compressed" | awk '{print $1}')

    [ -z "$blocksize" ] && blocksize=131072
    [ -z "$comp" ] && comp=xz

    mksquashfs "$src" "$out" \
        -noappend \
        -comp "$comp" \
        -b "$blocksize"
}

# ---- REBUILD ALL SQUASHFS ----
rebuild_sq romfs
rebuild_sq usr
rebuild_sq web
rebuild_sq custom
rebuild_sq logo

echo "[*] Creating empty flash image..."
dd if=/dev/zero of="$OUT" bs="$gcd" count="$FLASH_BLOCKS"

write_part() {
    file="$1"
    offset="$2"
    size="$3"

    echo "  → Writing $(basename "$file") @ $(printf 0x%x "$offset")"

    actual_size=$(stat -c%s "$file")
    if [ "$actual_size" -gt "$size" ]; then
        echo "ERROR: $file too large ($actual_size > $size)"
        exit 1
    fi

    dd if="$file" of="$OUT" \
       bs="$gcd" seek=$((offset / gcd)) conv=notrunc
}

# ---- RAW PARTITIONS ----
write_part binary/loader.bin        0x000000 0x10000
write_part binary/fdt.bin           0x010000 0x20000
write_part binary/fdt.restore.bin   0x030000 0x20000
write_part binary/uboot.bin         0x050000 0x60000

# ---- REBUILT SQUASHFS ----
write_part "$TMP_SQ/romfs.squashfs"   0x0b0000 0x370000
write_part "$TMP_SQ/usr.squashfs"     0x420000 0x780000
write_part "$TMP_SQ/web.squashfs"     0xba0000 0x70000
write_part "$TMP_SQ/custom.squashfs"  0xc10000 0x350000
write_part "$TMP_SQ/logo.squashfs"    0xf60000 0x20000

# ---- JFFS2 ----
write_part binary/mtd.jffs2          0xf80000 0x80000

echo "[✓] Firmware rebuilt: $OUT"
