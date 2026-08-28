#!/bin/sh
if [ $# -ne 2 ] 
then
	echo "Usage: basename essid key"
	exit 0
fi
echo "ESSID= $1"
echo "Key= $2"
iwpriv eth2 set NetworkType=Infra
iwpriv eth2 set AuthMode=SHARED
iwpriv eth2 set EncrypType=WEP
iwpriv eth2 set Key1=$2
iwpriv eth2 set DefaultKeyID=1
iwpriv eth2 set SSID="$1"
exit 0
