#!/bin/sh
# Restart ptc310_app 
ps | grep "ptc310_app" | grep -v "grep" | awk '{print $1}' | sed "s/^/kill -9 /" | sh
sleep 1
chmod 755 /root/app/ptc310_app
/root/app/ptc310_app ttyS2 ttyS1 > /root/sdcard/ptc310_app_log.log &
