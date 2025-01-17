#!/bin/sh
ps | grep "goahead" | grep -v "grep" | awk '{print $1}' | sed "s/^/kill -9 /" | sh
# Kill all of the programs which used the /root/sdcard/
# Such as goahead, ssh conncetion etc.
fuser -m /root/sdcard | sed "s/^/kill -9 /" | sh
sleep 1
umount /root/sdcard
mount /dev/mmcblk0p2 /root/sdcard/


