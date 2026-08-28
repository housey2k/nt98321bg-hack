#!/usr/bin/env bash
set -e

CROSS_COMPILE="arm-ca9-linux-uclibcgnueabihf-"
CPP="${CROSS_COMPILE}cpp"

SDK_ROOT="/home/brenno/NT9832x_SDK/software/board/na51068_linux_sdk"
NVT_HDAL_DIR="${SDK_ROOT}/code/hdal"

# Target DTS base name (without .dts extension)
DTS_NAME="${1:-cfg_DVR_4CH}"

echo "Processing ${DTS_NAME}.dts..."

# Preprocess
${CPP} -I. \
  -I${NVT_HDAL_DIR}/include \
  -I${NVT_HDAL_DIR}/vendor/isp/include \
  -nostdinc -undef -x assembler-with-cpp \
  "${DTS_NAME}.dts" > "${DTS_NAME}.tmp.dts"

# Compile to DTB
dtc -O dtb -b 0 -o "${DTS_NAME}.dtb" "${DTS_NAME}.tmp.dts"

rm "${DTS_NAME}.tmp.dts"

echo "Successfully generated ${DTS_NAME}.dtb ("$(stat -c%s "${DTS_NAME}.dtb")" bytes)"
