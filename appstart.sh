#!/bin/sh

# add your app start script here

export PATH="$PATH:/root/bin/"

echo "nameserver 114.114.114.114" > /etc/resolv.conf

# mkdir -p /root/sdcard/
# mount /dev/mmcblk0p2 /root/sdcard/
mkdir -p /root/sdcard/
mount /dev/mmcblk0p1 /root/sdcard/
mkdir -p /root/sdcard2/
mount /dev/mmcblk0p2 /root/sdcard2/


# sh /root/bin/run.sh

sh /root/start_ethernet.sh &

echo "****************************************************"
echo "*                                                  *"
echo "*    www.csgsm.com  cstx.taobao.com     *"
echo "*                                                  *"
echo "****************************************************"

exit 0
