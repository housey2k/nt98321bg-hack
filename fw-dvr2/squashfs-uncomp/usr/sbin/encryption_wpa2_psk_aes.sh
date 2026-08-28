#!/bin/sh
if [ $# -ne 2 ] 
then
	echo "Usage:basename essid key"
	exit 0
fi
echo "ESSID=$1"
echo "Key=$2"
iwpriv eth2 set NetworkType=Infra
iwpriv eth2 set AuthMode=WPA2PSK
iwpriv eth2 set EncrypType=AES
iwpriv eth2 set SSID="$1"
iwpriv eth2 set WPAPSK=$2
iwpriv eth2 set SSID="$1"
exit 1
