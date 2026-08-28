#!/bin/sh
set -e

FW=firmware.bin
BS=16

mkdir -p binary work squashfs-uncomp

echo "[*] Extracting raw partitions..."

dd if=$FW bs=$BS skip=$((0x000000/$BS)) count=$((0x10000/$BS)) of=binary/loader.bin
dd if=$FW bs=$BS skip=$((0x010000/$BS)) count=$((0x20000/$BS)) of=binary/fdt.bin
dd if=$FW bs=$BS skip=$((0x030000/$BS)) count=$((0x20000/$BS)) of=binary/fdt.restore.bin
dd if=$FW bs=$BS skip=$((0x050000/$BS)) count=$((0x60000/$BS)) of=binary/uboot.bin
dd if=$FW bs=$BS skip=$((0x0f80000/$BS)) count=$((0x80000/$BS)) of=binary/mtd.jffs2

echo "[*] Extracting SquashFS images to work/..."

dd if=$FW bs=$BS skip=$((0x0b0000/$BS)) count=$((0x370000/$BS)) of=work/romfs.squashfs
dd if=$FW bs=$BS skip=$((0x420000/$BS)) count=$((0x780000/$BS)) of=work/usr.squashfs
dd if=$FW bs=$BS skip=$((0x0ba0000/$BS)) count=$((0x70000/$BS)) of=work/web.squashfs
dd if=$FW bs=$BS skip=$((0x0c10000/$BS)) count=$((0x350000/$BS)) of=work/custom.squashfs
dd if=$FW bs=$BS skip=$((0x0f60000/$BS)) count=$((0x20000/$BS)) of=work/logo.squashfs

echo "[*] Unpacking SquashFS filesystems..."

extract_fs() {
    img="$1"
    name="$2"

    rm -rf squashfs-root "squashfs-uncomp/$name"
    unsquashfs "$img"
    mv squashfs-root "squashfs-uncomp/$name"
}

extract_fs work/romfs.squashfs  romfs
extract_fs work/usr.squashfs    usr
extract_fs work/web.squashfs    web
extract_fs work/custom.squashfs custom
extract_fs work/logo.squashfs   logo

echo "[*] Uncompressing U-Boot partition..."

python3 NTKFWinfo.py -i binary/uboot.bin -u 0

rm -rf work

echo "[✓] Unpack complete"
echo "  - Raw partitions: binary/"
echo "  - Filesystems:    squashfs-uncomp/"
