/*
 * dump_pool.c - LD_PRELOAD interposer to capture the REAL mem-pool
 * config struct Sofia builds and sends to the kernel at runtime.
 *
 * v2: writes raw to /dev/sda1 (bypassing whatever Sofia itself mounts
 * it as, so we don't need to know/guess its mountpoint), ALSO does a
 * single atomic write() of a hex dump to stderr (avoids interleaving
 * with Sofia's own concurrent prints), then abort()s the process so
 * nothing else can write to the terminal afterward.
 *
 * Build:
 *   arm-ca9-linux-uclibcgnueabihf-gcc -shared -fPIC dump_pool.c \
 *       -o dump_pool.so -ldl
 *
 * Run:
 *   LD_PRELOAD=/bin/dump_pool.so ./Sofia
 *
 * Output:
 *   - raw bytes written to /dev/sda1 starting at offset 0, prefixed
 *     with an 8-byte magic + 4-byte length header so you can locate
 *     it unambiguously even if the partition had other data before.
 *   - a hex dump written to stderr in ONE write() syscall (no
 *     interleaving with Sofia's own threads/prints).
 *   - process is then abort()ed - Sofia will not continue past this
 *     point, so nothing else writes to the terminal afterward.
 *
 * WARNING: this OVERWRITES THE START OF /dev/sda1 with our dump.
 * Whatever filesystem/data is currently on that partition WILL be
 * clobbered at the offset we write to. Only point this at a
 * disposable/scratch USB stick, not one with data you need.
 *
 * Override the target device via env var DUMP_DEV if /dev/sda1 isn't
 * right for your setup, e.g.:
 *   DUMP_DEV=/dev/sda LD_PRELOAD=/bin/dump_pool.so ./Sofia
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/mount.h>

#define TARGET_CMD    0x5000564aUL
#define DUMP_SIZE     8192
#define MAGIC         "POOLDUMP"   /* 8 bytes */
#define MAGIC_LEN     8
#define DEFAULT_DEV   "/dev/sda1"

static int (*real_ioctl)(int, unsigned long, ...) = NULL;

/* Build a full hex dump into `out` (caller-provided buffer), returns
 * length written. No stdio used here - keeps this a single buffer we
 * can push out with one write() syscall for atomicity. */
static size_t build_hexdump(const unsigned char *buf, size_t len, char *out, size_t outcap)
{
    static const char hex[] = "0123456789abcdef";
    size_t o = 0, i, j;

    for (i = 0; i < len && o + 80 < outcap; i += 16) {
        char off[4];
        int k;
        for (k = 3; k >= 0; k--) {
            off[3 - k] = hex[(i >> (k * 4)) & 0xF];
        }
        memcpy(out + o, off, 4); o += 4;
        out[o++] = ':';
        out[o++] = ' ';

        for (j = 0; j < 16; j++) {
            if (i + j < len) {
                unsigned char b = buf[i + j];
                out[o++] = hex[b >> 4];
                out[o++] = hex[b & 0xF];
            } else {
                out[o++] = ' ';
                out[o++] = ' ';
            }
            out[o++] = ' ';
        }
        out[o++] = '\n';
    }
    return o;
}

static void dump_to_device(const unsigned char *buf, size_t len)
{
    const char *dev = getenv("DUMP_DEV");
    int fd;
    unsigned char header[MAGIC_LEN + 4];
    unsigned char full[MAGIC_LEN + 4 + DUMP_SIZE];

    if (!dev || !dev[0])
        dev = DEFAULT_DEV;

    /* Best-effort blind unmount in case Sofia already has it mounted
     * somewhere - ignore failure, we're going to write raw regardless. */
    umount2(dev, MNT_FORCE);

    fd = open(dev, O_WRONLY | O_SYNC);
    if (fd < 0) {
        char msg[256];
        int n = snprintf(msg, sizeof(msg),
                          "[dump_pool] FAILED to open %s for raw write\n",
                          dev);
        write(2, msg, n);
        return;
    }

    memcpy(header, MAGIC, MAGIC_LEN);
    header[MAGIC_LEN + 0] = (unsigned char)(len & 0xFF);
    header[MAGIC_LEN + 1] = (unsigned char)((len >> 8) & 0xFF);
    header[MAGIC_LEN + 2] = (unsigned char)((len >> 16) & 0xFF);
    header[MAGIC_LEN + 3] = (unsigned char)((len >> 24) & 0xFF);

    memcpy(full, header, sizeof(header));
    memcpy(full + sizeof(header), buf, len);

    lseek(fd, 0, SEEK_SET);
    write(fd, full, sizeof(header) + len);
    fsync(fd);
    close(fd);
}

int ioctl(int fd, unsigned long request, ...)
{
    va_list ap;
    void *arg;

    if (!real_ioctl) {
        real_ioctl = dlsym(RTLD_NEXT, "ioctl");
        if (!real_ioctl) {
            write(2, "[dump_pool] FATAL: could not resolve real ioctl\n", 50);
            _exit(1);
        }
    }

    va_start(ap, request);
    arg = va_arg(ap, void *);
    va_end(ap);

    if (request == TARGET_CMD && arg != NULL) {
        static char hexbuf[DUMP_SIZE * 4];
        size_t hexlen;
        char note[160];
        int n;

        n = snprintf(note, sizeof(note),
                     "\n[dump_pool] Intercepted mem_init ioctl! cmd=0x%08lx buf=%p\n",
                     request, arg);
        write(2, note, n);

        /* 1) raw dump straight to the block device, bypassing Sofia's
         *    own mount entirely */
        dump_to_device((const unsigned char *)arg, DUMP_SIZE);

        n = snprintf(note, sizeof(note),
                     "[dump_pool] Wrote %d bytes (+8-byte magic +4-byte len header) to %s\n",
                     DUMP_SIZE, getenv("DUMP_DEV") ? getenv("DUMP_DEV") : DEFAULT_DEV);
        write(2, note, n);

        /* 2) single atomic write() of the full hex dump to stderr -
         *    no fprintf/printf here, so no interleaving with Sofia's
         *    own concurrent output */
        hexlen = build_hexdump((const unsigned char *)arg, 768, hexbuf, sizeof(hexbuf));
        write(2, hexbuf, hexlen);

        n = snprintf(note, sizeof(note),
                     "[dump_pool] Dump complete. Aborting process now.\n");
        write(2, note, n);

        /* 3) stop everything right here so nothing else can write to
         *    the terminal or touch the device after us */
        abort();
    }

    return real_ioctl(fd, request, arg);
}
