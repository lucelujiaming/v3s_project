#!/bin/sh

# ps | grep svm | grep -v "grep" | awk '{print $1}' | sed "s/^/kill -9 /" 
if [ $# == 1 ] ; then
	echo -n $1 > /data/svm/current_mac
	/usr/bin/killall5 udhcpc
	IPADDR=192.168.168.$1
	/sbin/ifconfig eth0 down
	/sbin/ifconfig eth0 hw ether $1
	/sbin/ifconfig eth0 up 
	# kill /tools/svm /data/svm/app.scode /data/svm/app.sab
	ps | grep "/tools/svm" | grep -v "grep" | awk '{print $1}' | sed "s/^/kill -9 /" | sh
	# Restart telnetd.
	killall telnetd
	sleep 1
	/etc/rc.d/rc.boot
	# kill goahead
	ps | grep "goahead" | grep -v "grep" | awk '{print $1}' | sed "s/^/kill -9 /" | sh
	/bin/goahead --verbose --home /www > /data/svm/goahead_output.log &
    sync
	sleep 1
else
    echo "NG"
fi

