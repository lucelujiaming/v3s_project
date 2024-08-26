#!/bin/sh

# ps | grep svm | grep -v "grep" | awk '{print $1}' | sed "s/^/kill -9 /" 
if [ $# == 2 ] ; then
	echo -n $1 > /root/app/current_first_mac
	echo -n $2 > /root/app/current_second_mac
	/usr/bin/killall5 udhcpc
	# Set eth0
	/sbin/ifconfig eth0 down
	/sbin/ifconfig eth0 hw ether $1
	/sbin/ifconfig eth0 up 
	# Set eth1
	/sbin/ifconfig eth1 down
	/sbin/ifconfig eth1 hw ether $2
	/sbin/ifconfig eth1 up 
	# kill /tools/svm /data/svm/app.scode /data/svm/app.sab
	ps | grep "/tools/svm" | grep -v "grep" | awk '{print $1}' | sed "s/^/kill -9 /" | sh
	# Restart telnetd.
	killall telnetd
	sleep 1
	/etc/rc.d/rc.boot
	# kill goahead
	ps | grep "goahead" | grep -v "grep" | awk '{print $1}' | sed "s/^/kill -9 /" | sh
        /root/app/goahead --verbose --home /root/app/www  > /root/sdcard/app/goahead_output.log &
	sleep 1
else
    echo "NG"
fi

