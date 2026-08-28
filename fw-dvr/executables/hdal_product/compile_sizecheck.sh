#!/usr/bin/env bash
set -e

# ==============================================================================
# 1. SDK & Cross-Compiler Environment Configuration
# ==============================================================================
export CC=arm-ca9-linux-uclibcgnueabihf-gcc
export STRIP=arm-ca9-linux-uclibcgnueabihf-strip
export NVT_PRJCFG_CFG=Linux

export SDK_ROOT="/home/brenno/NT9832x_SDK/na51068_linux_sdk_patched"
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

${STRIP} module_init

echo "=================================================="
echo "Build complete! Checking dynamic dependencies:"
echo "=================================================="
readelf -d module_init | grep NEEDED

# ==============================================================================
# 4. sizecheck.c - ABI/struct-layout sanity probe
#
# Rebuilds module_init.o in a dry-run to capture the EXACT compiler invocation
# (all -I include paths, -D defines, etc.) Make used for it, then reuses that
# same command line verbatim against sizecheck.c. This guarantees sizecheck.c
# sees hdal.h under identical preprocessor conditions as the real build,
# instead of us guessing include paths by hand.
# ==============================================================================
echo "=================================================="
echo "Building sizecheck (struct-layout probe)..."
echo "=================================================="

if [ ! -f sizecheck.c ]; then
    echo "sizecheck.c not found in $(pwd) - skipping sizecheck build."
    echo "Place sizecheck.c next to module_init.c and rerun this script to include it."
else
    # Force module_init.o to be considered stale so the dry-run actually
    # prints a real compile line for it (not a 'nothing to be done' no-op).
    touch module_init.c

    # IMPORTANT: temporarily disable -e around this block. If grep finds no
    # match (e.g. the Makefile's recipe doesn't look like we guessed), grep
    # exits 1, which under `set -e` kills the WHOLE SCRIPT right here with
    # zero output - looks like it "just stops". We don't want that; we want
    # to see the diagnostic message below instead.
    set +e
    MAKE_DRYRUN_OUTPUT="$(make -Bnw WARNING="${WARNING_FLAGS}" module_init.o 2>&1)"
    echo "---- make -Bn output (for debugging) ----"
    echo "${MAKE_DRYRUN_OUTPUT}"
    echo "------------------------------------------"
    # Match on "module_init.c" alone - it's unique to the compile recipe line
    # (the later link step only ever mentions module_init.o, never the .c),
    # so we don't need to also anchor on where -c happens to sit in the line.
    RECIPE_LINE="$(echo "${MAKE_DRYRUN_OUTPUT}" | grep -m1 -F 'module_init.c')"
    set -e

    if [ -z "${RECIPE_LINE}" ]; then
        echo "Could not extract module_init.o compile line from 'make -Bn' output."
        echo "Falling back to no-flags compile - this will likely fail to find hdal.h."
        echo "Paste me the '---- make -Bn output ----' block above (or the Makefile)"
        echo "so I can fix the grep pattern to match your Makefile's actual recipe."
        SIZECHECK_CMD="${CC} sizecheck.c -o sizecheck"
    else
        echo "Reusing captured compile flags:"
        echo "  ${RECIPE_LINE}"
        # Swap the source filename, drop the "-c" (we want a full link, not just
        # an object file) and redirect "-o module_init.o" to "-o sizecheck".
        # Token order here is "module_init.c -c -o module_init.o", not the more
        # common "-c module_init.c -o module_init.o", so match -c as a lone word.
        SIZECHECK_CMD="$(echo "${RECIPE_LINE}" \
            | sed -e 's/module_init\.c/sizecheck.c/' \
                  -e 's/-o[[:space:]]*module_init\.o/-o sizecheck/' \
                  -e 's/[[:space:]]-c[[:space:]]/ /')"
    fi

    echo "Running: ${SIZECHECK_CMD}"
    eval "${SIZECHECK_CMD}"

    echo "=================================================="
    echo "sizecheck built. ABI attributes vs pif.o:"
    echo "=================================================="
    arm-linux-gnueabihf-readelf -A sizecheck.o 2>/dev/null || readelf -A sizecheck.o

    echo "=================================================="
    echo "NOTE: sizecheck is an ARM binary and can't run natively on this host."
    echo "Copy it to the target device (or run under qemu-arm) and execute it:"
    echo "    ./sizecheck"
    echo "Compare 'sizeof(HD_COMMON_MEM_INIT_CONFIG)' against 5632 (the size"
    echo "baked into the pif_set_mem_init ioctl command 0x5600564a)."
    echo "=================================================="
fi
