#!/bin/sh

# ps | grep svm | grep -v "grep" | awk '{print $1}' | sed "s/^/kill -9 /" 
if [ $# == 1 ] ; then
	echo -n $1 > /data/svm/current_ip
	/usr/bin/killall5 udhcpc
	IPADDR=$1
	/sbin/ifconfig eth0 down
	/sbin/ifconfig eth0 inet $IPADDR netmask 255.255.255.0 up
	# /bin/httpd -c /etc/httpd.conf
	# /usr/sbin/svcled 1>/dev/null 2>&1 &
	sleep 1
	killall telnetd
	sleep 1
	/etc/rc.d/rc.boot
else
    echo "NG"
fi

