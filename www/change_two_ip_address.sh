#!/bin/sh

# ps | grep svm | grep -v "grep" | awk '{print $1}' | sed "s/^/kill -9 /" 
if [ $# == 2 ] ; then
	echo -n $1 > /root/sdcard/app/current_first_ip
	echo -n $2 > /root/sdcard/app/current_second_ip
	/usr/bin/killall5 udhcpc
	FIRSTIPADDR=$1
	/sbin/ifconfig eth0 down
	/sbin/ifconfig eth0 inet $FIRSTIPADDR netmask 255.255.255.0 up
	SECONDIPADDR=$2
	/sbin/ifconfig eth1 down
	/sbin/ifconfig eth1 inet $SECONDIPADDR netmask 255.255.255.0 up
	# kill /tools/svm /data/svm/app.scode /data/svm/app.sab
	ps | grep "/tools/svm" | grep -v "grep" | awk '{print $1}' | sed "s/^/kill -9 /" | sh
	# Restart telnetd.
	killall telnetd
	sleep 1
	/etc/rc.d/rc.boot
	# kill goahead
	ps | grep "goahead" | grep -v "grep" | awk '{print $1}' | sed "s/^/kill -9 /" | sh
	/bin/goahead --verbose --home /www > /root/sdcard/app/goahead_output.log &
	sleep 1
else
    echo "NG"
fi

