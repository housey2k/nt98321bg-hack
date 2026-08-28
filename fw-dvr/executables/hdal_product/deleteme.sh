#!/usr/bin/env bash
set -e

# ==============================================================================
# 1. SDK & Cross-Compiler Environment Configuration
# ==============================================================================
export CC=arm-ca9-linux-uclibcgnueabihf-gcc
export STRIP=arm-ca9-linux-uclibcgnueabihf-strip
export NVT_PRJCFG_CFG=Linux

export SDK_ROOT="/home/brenno/NT9832x_SDK/software/board/na51068_linux_sdk"
export NVT_HDAL_DIR="${SDK_ROOT}/code/hdal"
export NVT_VOS_DIR="${SDK_ROOT}/code/vos"

# ==============================================================================
# 2. Build Flags Setup
# ==============================================================================
WARNING_FLAGS="-Wall -Wundef -Wno-missing-braces -Wstrict-prototypes"

SEARCH_PATHS="-L./libfdt \
-L${NVT_HDAL_DIR}/source \
-L${NVT_HDAL_DIR}/vendor/isp/source \
-L${NVT_HDAL_DIR}/drivers/u_driver/source/2d_lib \
-L${NVT_HDAL_DIR}/vendor/media/source"

# ==============================================================================
# 3. Compilation & Linking
# ==============================================================================
echo "=================================================="
echo "Building object files and libfdt.a..."
echo "=================================================="

# Compile libfdt static archive
make -C libfdt libfdt.a

# Compile .o files without letting Makefile run its final link step
make WARNING="${WARNING_FLAGS}" module_init.o show_logo.o

echo "=================================================="
echo "Linking module_init statically..."
echo "=================================================="

# Directly invoke CC with exact object order + static group flags
${CC} module_init.o show_logo.o \
    ${SEARCH_PATHS} \
    -Wl,-Bstatic \
    -Wl,--start-group -lhdal -lvendor_isp -lfdt -lgm2d -lvendor_media -Wl,--end-group \
    -Wl,-Bdynamic -lpthread \
    -o module_init

#${STRIP} module_init

echo "=================================================="
echo "Build complete! Checking dynamic dependencies:"
echo "=================================================="
readelf -d module_init | grep NEEDED
