#!/bin/sh
# Secona would change the RTC device, 
# so we have to stop svm and use /etc/rc.d/rc.svm to sync /dev/rtc0.
if [ $# == 1 ] ; then
	date -s $1
	sleep 1
	# kill /tools/svm and use /etc/rc.d/rc.svm to sync /dev/rtc0
	ps | grep "/tools/svm" | grep -v "grep" | awk '{print $1}' | sed "s/^/kill -9 /" | sh
else
    echo "NG"
fi

