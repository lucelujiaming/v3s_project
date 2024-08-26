#!/bin/sh

# mkdir -p /root/app/
top -b -n 1 -d 0 | head -2 > /root/sdcard/app/top_output.txt
cat /root/sdcard/app/top_output.txt | head -1 > /root/sdcard/app/board_meminfo.txt
cat /root/sdcard/app/top_output.txt | head -2 | tail -1 |  awk '{print $3*100 "%% in 1min, " $4*100 "%% in 5mins, " $5*100 "%% in 15mins."}' > /root/sdcard/app/board_cpuloadinfo.txt



