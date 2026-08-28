#!/bin/sh
# part of usb_modeswitch 1.1.6
	/sbin/rmmod /usr/lib/modules/misc/usbserial.ko
	/sbin/insmod /usr/lib/modules/misc/usbserial.ko vendor=0x$1 product=0x$2
