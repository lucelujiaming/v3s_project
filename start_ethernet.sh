#!/bin/sh
echo "lujiaming -----------------------------------"
# Enable framebuffer play 
ln -sf /dev/fb8  /dev/fb0
# Enable two ethernet 
/root/usb-set-hostmode.sh 
sleep 1
/root/usb-set-devicemode.sh
sleep 1
/root/usb-set-hostmode.sh 
echo "lujiaming -----------------------------------"
sleep 1
if [ ! -e "/root/app/current_first_ip" ]; then
    ifconfig eth0 192.168.168.129
else
    FIRSTIPADDR=`cat /root/app/current_first_ip`
    if [ $? -eq 0 ]; then
        echo "lujiaming --------------FIRSTIPADDR=$FIRSTIPADDR---------------------"
        ifconfig eth0 $FIRSTIPADDR
    else
        ifconfig eth0 192.168.168.129
    fi
fi

if [ ! -e "/root/app/current_second_ip" ]; then
    ifconfig eth1 192.168.168.130
else
    SECONDIPADDR=`cat /root/app/current_second_ip`
    if [ $? -eq 0 ]; then
        echo "lujiaming --------------SECONDIPADDR=$SECONDIPADDR---------------------"
        ifconfig eth1 $SECONDIPADDR
    else
        ifconfig eth1 192.168.168.130
    fi
fi

if [ -e "/root/app/current_first_mac" ]; then
    FIRSTMACADDR=`cat /root/app/current_first_mac`
    if [ $? -eq 0 ]; then
        echo "lujiaming --------------FIRSTMACADDR=$FIRSTMACADDR---------------------"
        /sbin/ifconfig eth0 down
        /sbin/ifconfig eth0 hw ether $FIRSTMACADDR
        /sbin/ifconfig eth0 up
    fi
fi

if [ -e "/root/app/current_second_mac" ]; then
    SECONDMACADDR=`cat /root/app/current_second_mac`
    if [ $? -eq 0 ]; then
        echo "lujiaming --------------SECONDMACADDR=$SECONDMACADDR---------------------"
        /sbin/ifconfig eth1 down
        /sbin/ifconfig eth1 hw ether $SECONDMACADDR
        /sbin/ifconfig eth1 up
    fi
fi

/root/app/goahead --verbose --home /root/app/www > /root/sdcard/app/goahead_output.log &
# Run sedona app.
if [ ! -e "/root/app/current_first_ip" ]; then
    echo "lujiaming --------------We need /root/app/current_first_ip ---------------------"
else
    cd /root/app/
    # ./svm app_134.scode app.sab >> svm_output.log &
    cd - 1>/dev/null 2>&1
fi
echo "lujiaming -----------------------------------"

top_count=0
while true; do
    svmresult=`ps | grep '/root/app/svm' | grep -v grep`
    if [ -z "$svmresult" ]; then
      if [ -e "/root/app/app.sab.stage" ]; then
        mv /root/app/app.sab.stage /root/app/app.sab
        rm -rf /root/app/m*.zip
      fi
      # Print date and hwclock. 
      # echo -n "Soft Clock : " && date && echo -n "Hard Clock : " && hwclock
      # Secona would change the RTC device, 
      # so we have to sync /dev/rtc0 by hwclock before start svm.
      hwclock -w
      sleep 1
      if [ ! -e "/root/app/current_first_ip" ]; then  
          cd /root/app/
          # echo "lujiaming --------------We need /root/app/current_first_ip ---------------------" 
          cd - 1>/dev/null 2>&1
      else                                                                                        
          cd /root/app/
          # ./svm app_134.scode app.sab >> svm_output.log &
          cd - 1>/dev/null 2>&1
      fi
      cd - 1>/dev/null 2>&1
      # The svm does not always run successfully
      echo -n "Not Running" > /root/sdcard/app/svm_info.txt
    else
      echo -n "Running" > /root/sdcard/app/svm_info.txt
    fi
    # echo "lujiaming --------------We need /root/app/current_first_ip ---------------------" 

    eth0result=`ifconfig eth0 | grep "inet addr"`
    if [ -z "$eth0result" ]; then
        if [ ! -e "/root/app/current_first_ip" ]; then
            ifconfig eth0 192.168.168.129
        else
            FIRSTIPADDR=`cat /root/app/current_first_ip`
            if [ $? -eq 0 ]; then
                echo "lujiaming --------------FIRSTIPADDR=$FIRSTIPADDR---------------------"
                ifconfig eth0 $FIRSTIPADDR
            else
                ifconfig eth0 192.168.168.129
            fi
        fi
    fi
    # echo "lujiaming --------------We need /root/app/current_first_ip ---------------------" 
    eth1result=`ifconfig eth1 | grep "inet addr"`
    if [ -z "$eth1result" ]; then
        if [ ! -e "/root/app/current_second_ip" ]; then
            ifconfig eth1 192.168.168.130
        else
            SECONDIPADDR=`cat /root/app/current_second_ip`
            if [ $? -eq 0 ]; then
                echo "lujiaming --------------SECONDIPADDR=$SECONDIPADDR---------------------"
                ifconfig eth1 $SECONDIPADDR
            else 
                ifconfig eth1 192.168.168.130                
            fi
        fi
    fi
    # echo "lujiaming --------------We need /root/app/current_second_ip ---------------------" 
    # Output top info
    if [ $top_count -gt 9 ]; then                         
        # top -b -n 1 -d 0 | head -2 > /data/svm/top_output.txt
        /root/app/www/output_topinfo.sh &
        sync
        top_count=0
    fi
    top_count=`expr $top_count + 1`
    # echo "lujiaming --------------We have top_count = $top_count ---------------------" 
    # end of Output top info 
    sleep 5
done

