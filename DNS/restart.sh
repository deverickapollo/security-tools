#!/bin/bash

FILENAME="/var/cache/bind/dump.db"
IP="192.168.0.7"

sudo rndc dumpdb -cache
if grep -q $IP $FILENAME
then
    # code if found
	echo "FOUND"
	grep $IP $FILENAME
else
    # code if not found
	echo "FUCK MY LIFE"
fi
echo
while true; do
    read -p "Now What? Y to flush; N to exit; M to monitor    " ynm
    case $ynm in
        [Yy]* ) sudo rndc flush; sudo rndc reload; sudo /etc/init.d/bind9 restart;sudo service bind9 restart; exit 1;;
        [Nn]* ) exit;;
        [Mm]* )echo;echo "Monitor Mode Started"; ( tail -f -n0 $FILENAME & ) | grep -q $IP;;
	* ) echo "Please answer yes or no.";;
    esac
done
