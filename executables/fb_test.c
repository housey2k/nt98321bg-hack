#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

int main() {
    int fb = open("/dev/fb0", O_RDWR);
    if (fb < 0) {
        perror("open");
        return 1;
    }

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("FBIOGET_VSCREENINFO");
        return 1;
    }

    if (ioctl(fb, FBIOGET_FSCREENINFO, &finfo)) {
        perror("FBIOGET_FSCREENINFO");
        return 1;
    }

    size_t screensize = finfo.smem_len;

    uint8_t *fbp = mmap(
        0,
        screensize,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fb,
        0
    );

    if (fbp == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    printf("Resolution: %ux%u, %ubpp\n",
           vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    /* Fill screen with red */
    for (unsigned y = 0; y < vinfo.yres; y++) {
        for (unsigned x = 0; x < vinfo.xres; x++) {
            long location =
                (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) +
                (y + vinfo.yoffset) * finfo.line_length;

            if (vinfo.bits_per_pixel == 32) {
                /* ARGB */
                fbp[location + 0] = 0x00; // B
                fbp[location + 1] = 0x00; // G
                fbp[location + 2] = 0xFF; // R
                fbp[location + 3] = 0x00; // A
            } else if (vinfo.bits_per_pixel == 16) {
                /* RGB565 */
                uint16_t red = 0xF800;
                *((uint16_t*)(fbp + location)) = red;
            }
        }
    }

    munmap(fbp, screensize);
    close(fb);
    return 0;
}
