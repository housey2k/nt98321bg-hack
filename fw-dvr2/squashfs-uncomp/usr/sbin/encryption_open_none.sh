#!/bin/sh
if [ $# -ne 1 ] 
then
	echo "Usage: basename essid key"
	exit 0
fi
echo "ESSID= $1"
#echo "Key= $2"

iwpriv eth2 set NetworkType=Infra
iwpriv eth2 set AuthMode=OPEN
iwpriv eth2 set EncrypType=NONE 
iwpriv eth2 set SSID="$1"
exit 0
