# Introduction
These are my (housey2k) efforts to reverse engineer a novatek NT98321BG based DVR

This environment is clean Linux, original apps (Sofia, dvrbox) all removed
to work on the original environment, please do:
```
cd ..
cp -r fw-dvr fw-dvr2
cd fw-dvr2
./unpack.sh
```

to enable UART on Linux kernel, enter U-Boot, enter the password on uboot-passwd.txt, then do:
```
setenv -f xmuart 0
saveenv
reset
```

To disable the watchdog on Linux, please do:
```
echo V > /dev/watchdog
```

scripts:
firmware.bin is the SPI dump
firmware-rebuilt.bin is firmware generated from build-img-*.sh
unpack.sh extracts firmware.bin (FSBL, U-Boot, squashfs, etc)
build-img.sh builds squashfs partitions only, replace manually and flash
build-img-breaking.sh builds squashfs and a full .bin file to flash, but breaks some stuff like u-boot logo, but the rest works normally
build-img-rootfs.sh builds only romfs and usr

fw-dvr is the workspace for modifying the clean firmware, only romfs and usr are present, vendor apps were removed
fw-dvr2 is the workspace for patching the original firmware, everything is preserved
fw-dvr-aaa is for extracting raw partitions such as U-Boot after modifying an env value
uboot is raw u-boot partitions with modified env values, the name of the files explain everything

any questions @housey2k on discord or GitHub
brennomaturino2@gmail.com

leaked SDK:
https://drive.google.com/file/d/1hHo7R6QZg4pVAegdWrWi6Kg5OoI4kU3V/view?usp=drive_link
https://archive.org/details/nt-9832-x
