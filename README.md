firmware.bin is the SPI dump
./unpack.sh extracts the FSBL, U-Boot, and squashfs partitions
./build-img.sh turns the folders back into squashfs files to be replaced manually
./build-img-breaking.sh builds a whole .bin ready to be flashed, but risks breaking stuff
