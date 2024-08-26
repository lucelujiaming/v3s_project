/********************************* Includes ***********************************/
#include "goahead.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <locale.h>
#include <sys/wait.h>
#include <unistd.h>

#include    "cJSON.h"

#define INSTRUMENT_INFO_STR_LEN     4096
#define BATTERY_INFO_STR_LEN        1024
#define USART_INFO_STR_LEN          1024

#define INFO_STR_LEN                5120

#define INFO_STR_LAST_COMMA         2

#define    TIME_SPAN               900 //   15mins  120       // 2 minutes
#define    TIME_SCALE              (24 * 60 * 60 / TIME_SPAN)

// Fireware decode macro and struct
#define FIRMWARE_MAGIC 0x55005678

#define FIRMWARE_SYSLOADER_KEY  's'
#define FIRMWARE_UBOOT_KEY      'u'
#define FIRMWARE_KERNEL_KEY     'k'
#define FIRMWARE_ROOTFS_KEY     'r'

#define FIRMWARE_SYSLOADER_FILENAME  "sramloader.bin"
#define FIRMWARE_UBOOT_FILENAME      "u-boot.bin"
#define FIRMWARE_KERNEL_FILENAME     "uImage"
#define FIRMWARE_ROOTFS_FILENAME     "rootfs.cramfs"

typedef struct {
    unsigned int magic;
    unsigned int len;
    unsigned short s_sum;
    unsigned short u_sum;
    unsigned short k_sum;
    unsigned short r_sum;
    unsigned int s_len;
    unsigned int u_len;
    unsigned int k_len;
    unsigned int r_len;
    char version[16];
    char btime[16];
} FIRMWARE_T;
// Fireware decode macro and struct end

static int get_cmd_printf(char *cmd, char *buf, int bufSize)
{
    FILE *fp;
    int status;
    char *p;
    char buffer[1024] = {0};
    int bufferSize = sizeof(buffer);
    int length = 0;
    int writeLen = 0;

    if (cmd == NULL || buf == NULL || bufSize <= 1)
        return -1;

    if ((fp = popen(cmd, "r")) == NULL) {
        return -1;
    }

    p = buffer;
    memset(buf, 0, bufSize);
    fgets(buffer, bufferSize, fp);
    do {
        writeLen = ((length + strlen(buffer)) > (bufSize - 1)) ? (bufSize - 1 - length) : strlen(buffer);

        memcpy(buf + length, buffer, writeLen);
        length += writeLen;
    } while (fgets(buffer, bufferSize, fp) != NULL);


    if (*(p = &buf[length - 1]) == 0x0A)
        *p = 0;

    status = pclose(fp);
    if (WIFEXITED(status)) {

        return WEXITSTATUS(status);
    }

    return -1;
}

char* get_last_modify_time(char* path, char* time_str)
{
    struct stat stat_buf;
    trace(2, "[%s:%s:%d] get_last_modify_time path = %s", __FILE__, __FUNCTION__, __LINE__, path);
    if(stat(path,&stat_buf)==0)
    {
        time_t last_modify_time=stat_buf.st_mtime;
        trace(2, "[%s:%s:%d] path = %s and last_modify_time = %ld"
            , __FILE__, __FUNCTION__, __LINE__, path, last_modify_time);
        sprintf(time_str, "%ld", last_modify_time);
        return ctime(&last_modify_time);
    }
    else
    {
        sprintf(time_str, "Unknown");
        trace(2, "[%s:%s:%d] path error", __FILE__, __FUNCTION__, __LINE__);
    }
    return NULL;
}

#define  APP_PATH_LEN    128
int get_ver_from_cmdline(char* cmd_line, char * ver_info)
{
    char *ver_start_pos = strstr(cmd_line, "ver=");
    if (ver_start_pos)
    {
        char *ver_end_pos = strchr(ver_start_pos + strlen("ver="), ' ');
        if (ver_end_pos)
        {
            strncpy(ver_info, ver_start_pos + strlen("ver="),
                ver_end_pos - ver_start_pos - strlen("ver="));
            return 1;
        }
        else
        {
            strcpy(ver_info, ver_start_pos + strlen("ver="));
            return 1;
        }
    }
    return 0;
}

int get_uboot_version(char * ver_info)
{
    char cmdline_string[APP_PATH_LEN] = "";
    memset(ver_info, 0x00, APP_PATH_LEN);
    get_cmd_printf("cat /proc/cmdline", cmdline_string, APP_PATH_LEN);

    return get_ver_from_cmdline(cmdline_string, ver_info);
}

int get_kernel_version(char * ver_info)
{
    char kernel_name_string[APP_PATH_LEN] = "";
    memset(kernel_name_string, 0x00, APP_PATH_LEN);
    get_cmd_printf("uname -s", kernel_name_string, APP_PATH_LEN);

    char kernel_release_string[APP_PATH_LEN] = "";
    memset(kernel_release_string, 0x00, APP_PATH_LEN);
    get_cmd_printf("uname -r", kernel_release_string, APP_PATH_LEN);

    char kernel_version_string[APP_PATH_LEN] = "";
    memset(kernel_version_string, 0x00, APP_PATH_LEN);
    get_cmd_printf("uname -v", kernel_version_string, APP_PATH_LEN);

    sprintf(ver_info, "%s %s %s", kernel_name_string, kernel_release_string, kernel_version_string);
    return 1;
}

int get_machine_name(char * machine_name_info)
{
    get_cmd_printf("uname -m", machine_name_info, APP_PATH_LEN);
    return 1;
}

char* initProc()
{
    char *pVal;
    char *ret_str;
#ifdef USE_CJSON
    cJSON *ret_data;
    char info_str[INFO_STR_LEN];

    ret_data = cJSON_CreateObject();

    memset(info_str, 0x00, INFO_STR_LEN);
    get_cmd_printf("cat /etc/VERSION", info_str, INFO_STR_LEN);
    cJSON_AddStringToObject(ret_data, "version", info_str);

    memset(info_str, 0x00, INFO_STR_LEN);
    pVal = get_last_modify_time("/root/app/app.sab", info_str);
    if(pVal)
        cJSON_AddStringToObject(ret_data, "buildtime", info_str);
    else
        cJSON_AddStringToObject(ret_data, "buildtime", "Unknown");

    memset(info_str, 0x00, INFO_STR_LEN);
    get_cmd_printf("ps | grep 'app.scode' | grep -v grep", info_str, INFO_STR_LEN);
    if(strlen(info_str) >= strlen("app.scode"))
        cJSON_AddStringToObject(ret_data, "status", "standby");
    else
        cJSON_AddStringToObject(ret_data, "status", "shutdown");

    ret_str = cJSON_Print(ret_data);
    cJSON_Delete(ret_data);
#else
    char version_string[APP_PATH_LEN] = "";
    char filetime_string[APP_PATH_LEN] = "";
    char status_string[APP_PATH_LEN] = "";

    char uboot_version[APP_PATH_LEN] = "";
    char kernel_version[APP_PATH_LEN] = "";
    char machine_name[APP_PATH_LEN] = "";

    get_cmd_printf("cat /etc/VERSION", version_string, APP_PATH_LEN);
    pVal = get_last_modify_time("/root/app/app.sab", filetime_string);
    get_cmd_printf("ps | grep 'app.scode' | grep -v grep", status_string, APP_PATH_LEN);
    if(strlen(status_string) >= strlen("app.scode"))
    {
        strcpy(status_string, "standby");
    }
    else
    {
        strcpy(status_string, "shutdown");
    }

    int iRet = get_uboot_version(uboot_version);
    if(iRet == 0)
    {
        strcpy(uboot_version, "unknown");
    }
    iRet = get_kernel_version(kernel_version);
    if(iRet == 0)
    {
        strcpy(kernel_version, "unknown");
    }
    iRet = get_machine_name(machine_name);

    ret_str = (char *)malloc(1024);
    if(ret_str)
    {
        sprintf(ret_str, "{ \"version\": \"%s\", \"buildtime\": \"%s\", \"status\": \"%s\", \"uboot\": \"%s\", \"kernel\": \"%s\", \"machine\": \"%s\" }",
           version_string, filetime_string, status_string, uboot_version, kernel_version, machine_name);
    }
#endif
    trace(2, "[%s:%s:%d] ret_str = %s", __FILE__, __FUNCTION__, __LINE__, ret_str);
    return ret_str;
}

void statusProc(Webs *wp)
{
    char *ret_str;
    char *pMode;
    char *pValue;
    int   iValue;
    char info_str[INFO_STR_LEN] = { 0 };
    char info_first_str[64] = { 0 };
    char info_second_str[64] = { 0 };
    char * pSeq = NULL;

    // char *pStartDateTime, *pEndDateTime;
    char *pRecordDateTime;

    pMode = websGetVar(wp, "mode", "");
    trace(2, "[%s:%s:%d] statusProc::pVal = %s", __FILE__, __FUNCTION__, __LINE__, pMode);

    if(strcmp(pMode, "init") == 0)
    {
        ret_str = initProc();
        if(ret_str)
        {
            websSetStatus(wp, 200);
            websWriteHeaders(wp, -1, 0);
            websWriteEndHeaders(wp);
            websWrite(wp, ret_str);
            websFlush(wp);
            free(ret_str);
        }
        websDone(wp);
    }
    else if(strcmp(pMode, "set_ipaddress") == 0)
    {
        pValue = websGetVar(wp, "value", "");
        trace(2, "[%s:%s:%d] statusProc::pValue = %s", __FILE__, __FUNCTION__, __LINE__, pValue);
        websDone(wp);

        pSeq = strchr(pValue, ';');
        if(pSeq)
        {
            memcpy(info_first_str, pValue, pSeq - pValue);
            memcpy(info_second_str, pSeq + 1, pValue + strlen(pValue) - pSeq - 1);
                sprintf(info_str, "/root/app/www/change_two_ip_address.sh %s %s &", 
                    info_first_str, info_second_str);
            system(info_str);
        }
        // change_ip_address.sh would restart goahead, so we need not return websDone
    }
    else if(strcmp(pMode, "set_macaddress") == 0)
    {
        pValue = websGetVar(wp, "value", "");
        trace(2, "[%s:%s:%d] statusProc::pValue = %s", __FILE__, __FUNCTION__, __LINE__, pValue);
        websDone(wp);

        pSeq = strchr(pValue, ';');
        if(pSeq)
        {
            memcpy(info_first_str, pValue, pSeq - pValue);
            system(info_str);
            memcpy(info_second_str, pSeq + 1, pValue + strlen(pValue) - pSeq - 1);
            sprintf(info_str, "/root/app/www/change_two_mac_address.sh %s %s &", 
                    info_first_str, info_second_str);
            system(info_str);
        }
        // change_ip_address.sh would restart goahead, so we need not return websDone
    }
    else if(strcmp(pMode, "set_datetime") == 0)
    {
        pValue = websGetVar(wp, "value", "");
        trace(2, "[%s:%s:%d] statusProc::pValue = %s", __FILE__, __FUNCTION__, __LINE__, pValue);
        websDone(wp);
        // Secona would change the RTC device,
        // so we have to stop svm and use /etc/rc.d/rc.svm to sync /dev/rtc0.
        sprintf(info_str, "/root/app/www/change_datetime.sh %s &", pValue);
        system(info_str);
    }
    else if(strcmp(pMode, "get_ipconfig") == 0)
    {
        get_cmd_printf("ifconfig eth0 | grep 'inet addr' | sed 's/  Bcast.*//' | sed 's/.*inet addr://'", info_first_str, 64);
        get_cmd_printf("ifconfig eth1 | grep 'inet addr' | sed 's/  Bcast.*//' | sed 's/.*inet addr://'", info_second_str, 64);
        sprintf(info_str, "%s;%s", info_first_str, info_second_str);
        // Use default value
        if(strlen(info_str) == 0)
        {
            strcpy(info_str, "192.168.168.129;192.168.168.130");
        }
        websSetStatus(wp, 200);
        websWriteHeaders(wp, -1, 0);
        websWriteEndHeaders(wp);
        websWrite(wp, info_str);
        websFlush(wp);
        websDone(wp);
    }
    else if(strcmp(pMode, "get_macconfig") == 0)
    {
        // MAC地址如果没有设定。给出默认值。不能留空。
        // get_cmd_printf("cat /root/app/current_mac", info_str, 1024);
        get_cmd_printf("ifconfig eth0 | grep HWaddr | sed 's/.*HWaddr //'", info_first_str, 64);
        get_cmd_printf("ifconfig eth1 | grep HWaddr | sed 's/.*HWaddr //'", info_second_str, 64);
        sprintf(info_str, "%s;%s", info_first_str, info_second_str);
        websSetStatus(wp, 200);
        websWriteHeaders(wp, -1, 0);
        websWriteEndHeaders(wp);
        websWrite(wp, info_str);
        websFlush(wp);
        websDone(wp);
    }
    else if(strcmp(pMode, "get_date") == 0)
    {
        get_cmd_printf("date +'%G/%m/%d %H:%M:%S'", info_str, INFO_STR_LEN);
        websSetStatus(wp, 200);
        websWriteHeaders(wp, -1, 0);
        websWriteEndHeaders(wp);
        websWrite(wp, info_str);
        websFlush(wp);
        websDone(wp);
    }
    else if(strcmp(pMode, "get_systeminfo") == 0)
    {
        char cpuload_string[APP_PATH_LEN] = "";
        char mem_string[APP_PATH_LEN] = "";
        char svm_string[APP_PATH_LEN] = "";

        get_cmd_printf("cat /root/sdcard/app/board_cpuloadinfo.txt", cpuload_string, APP_PATH_LEN);
        get_cmd_printf("cat /root/sdcard/app/board_meminfo.txt", mem_string, APP_PATH_LEN);
        get_cmd_printf("cat /root/sdcard/app/svm_info.txt", svm_string, APP_PATH_LEN);

        sprintf(info_str, "{ \"svm\": \"%s\", \"cpuload\": \"%s\", \"mem\": \"%s\" }",
           svm_string, cpuload_string, mem_string);
        websSetStatus(wp, 200);
        websWriteHeaders(wp, -1, 0);
        websWriteEndHeaders(wp);
        websWrite(wp, info_str);
        websFlush(wp);
        websDone(wp);
    }
    else if(strcmp(pMode, "get_history_datelist") == 0)
    {
        char datelist_string[APP_PATH_LEN] = "";
        sprintf(info_first_str, 
           "ls -d /root/sdcard/app/instrument_info/2*_*_* | sed 's/\\\///' | sed 's/.*info\\\///' | sed 's/\\\(.*\\\)/ \\\"\\1\\\",/'");
        trace(2, "[%s:%s:%d] info_first_str = %s", __FILE__, __FUNCTION__, __LINE__, info_first_str);
        get_cmd_printf(info_first_str, datelist_string, APP_PATH_LEN);
        // Remove last ","
        datelist_string[strlen(datelist_string) - 1] = '\0';
        sprintf(info_str, "{ \"date\": [ %s ] }", datelist_string);
        
        websSetStatus(wp, 200);
        websWriteHeaders(wp, -1, 0);
        websWriteEndHeaders(wp);
        websWrite(wp, info_str);
        websFlush(wp);
        
        websDone(wp);
    }
    else if(strcmp(pMode, "get_instrument_info") == 0)
    {
        char *instrument_info_string_ptr = (char *)malloc(INSTRUMENT_INFO_STR_LEN);
        char *battery_info_string_ptr = (char *)malloc(BATTERY_INFO_STR_LEN);
        char *usart_info_string_ptr = (char *)malloc(USART_INFO_STR_LEN);

        pRecordDateTime = websGetVar(wp, "record_time", "");
        if(strlen(pRecordDateTime) > 0)
        {
            int iInstrumentInfoFileSize = 0;
            struct stat stInstrumentInfoFile;
            sprintf(info_first_str, 
                "/root/sdcard/app/instrument_info/%s/instrument_history_info_record_unixtime_%s.txt", 
                pRecordDateTime, pRecordDateTime);
            if(stat(info_first_str, &stInstrumentInfoFile) == 0)
			{
                iInstrumentInfoFileSize = stInstrumentInfoFile.st_size;
            }
            trace(2, "[%s:%s:%d] /root/sdcard/app/instrument_info/%s/instrument_history_info_record_unixtime_%s.txt = %d", 
                    __FILE__, __FUNCTION__, __LINE__, 
                    pRecordDateTime, pRecordDateTime, iInstrumentInfoFileSize);
			
            int iUsartInfoFileSize = 0;
            struct stat stUsartInfoFile;
            sprintf(info_first_str, 
                "/root/sdcard/app/instrument_info/%s/usart_info_record_unixtime_%s.txt", 
                pRecordDateTime, pRecordDateTime);
            if(stat(info_first_str, &stUsartInfoFile) == 0)
			{
                iUsartInfoFileSize = stUsartInfoFile.st_size;
            }
            trace(2, "[%s:%s:%d] /root/sdcard/app/instrument_info/%s/usart_info_record_unixtime_%s.txt = %d", 
                    __FILE__, __FUNCTION__, __LINE__, 
                    pRecordDateTime, pRecordDateTime, iUsartInfoFileSize);
			
            int iBatteryInfoFileSize = 0;
            struct stat stBatteryInfoFile;
            sprintf(info_first_str, 
                "/root/sdcard/app/instrument_info/%s/battery_info_record_unixtime_%s.txt", 
                pRecordDateTime, pRecordDateTime);
            if(stat(info_first_str, &stBatteryInfoFile) == 0)
			{
                iBatteryInfoFileSize = stBatteryInfoFile.st_size;
            }
            trace(2, "[%s:%s:%d] /root/sdcard/app/instrument_info/%s/battery_info_record_unixtime_%s.txt = %d", 
                    __FILE__, __FUNCTION__, __LINE__, 
                    pRecordDateTime, pRecordDateTime, iBatteryInfoFileSize);
			
			if(((iInstrumentInfoFileSize + iUsartInfoFileSize + iBatteryInfoFileSize) < INFO_STR_LEN)
                && (iInstrumentInfoFileSize > 0) && (iUsartInfoFileSize > 0) && (iBatteryInfoFileSize > 0))
			{
                sprintf(info_first_str, 
                    "cat /root/sdcard/app/instrument_info/%s/instrument_history_info_record_unixtime_%s.txt", 
                    pRecordDateTime, pRecordDateTime);
	            get_cmd_printf(info_first_str, instrument_info_string_ptr, INSTRUMENT_INFO_STR_LEN);
                // Remove last ",\r\n"
                instrument_info_string_ptr[strlen(instrument_info_string_ptr) - INFO_STR_LAST_COMMA] = '\0';
                trace(2, "[%s:%s:%d] instrument_info_string_ptr = %s", __FILE__, __FUNCTION__, __LINE__, instrument_info_string_ptr);
				
                sprintf(info_first_str, 
                    "cat /root/sdcard/app/instrument_info/%s/usart_info_record_unixtime_%s.txt", 
                    pRecordDateTime, pRecordDateTime);
	            get_cmd_printf(info_first_str, battery_info_string_ptr, BATTERY_INFO_STR_LEN);
                // Remove last ",\r\n"
                battery_info_string_ptr[strlen(battery_info_string_ptr) - INFO_STR_LAST_COMMA] = '\0';
                trace(2, "[%s:%s:%d] battery_info_string_ptr = %s", __FILE__, __FUNCTION__, __LINE__, battery_info_string_ptr);
				
                sprintf(info_first_str, 
                    "cat /root/sdcard/app/instrument_info/%s/battery_info_record_unixtime_%s.txt", 
                    pRecordDateTime, pRecordDateTime);
	            get_cmd_printf(info_first_str, usart_info_string_ptr, USART_INFO_STR_LEN);
                // Remove last ",\r\n"
                usart_info_string_ptr[strlen(usart_info_string_ptr) - INFO_STR_LAST_COMMA] = '\0';
			}
            else
            {
                sprintf(instrument_info_string_ptr, "[0,0,0,0,0,0,0,0,0,0]");
                sprintf(battery_info_string_ptr, "0");
                sprintf(usart_info_string_ptr, "0");
            }
            snprintf(info_str, INFO_STR_LEN, 
                "{  \"Scale\": %d, \"InstrumentInfo\": [ %s ], \r\n\"BatteryInfo\": [ %s ], \r\n\"UsartInfo\": [ %s ] }", 
                TIME_SCALE, instrument_info_string_ptr, battery_info_string_ptr, usart_info_string_ptr);
            
            trace(2, "[%s:%s:%d] info_str = %s", __FILE__, __FUNCTION__, __LINE__, info_str);
            websSetStatus(wp, 200);
            websWriteHeaders(wp, -1, 0);
            websWriteEndHeaders(wp);
            websWrite(wp, info_str);
            websFlush(wp);
        }
        websDone(wp);
		free(instrument_info_string_ptr);
		free(battery_info_string_ptr);
		free(usart_info_string_ptr);
    }
    else if(strcmp(pMode, "get_battery_info") == 0)
    {
        char *battery_info_string_ptr = (char *)malloc(BATTERY_INFO_STR_LEN);
        pRecordDateTime = websGetVar(wp, "record_time", "");
        trace(2, "%ld - [%s:%s:%d] websWrite::pRecordDateTime = %s",
                      time(NULL), __FILE__, __FUNCTION__, __LINE__, pRecordDateTime);
        if(strlen(pRecordDateTime) > 0)
        {
            int iBatteryInfoFileSize = 0;
            struct stat stBatteryInfoFile;
            sprintf(info_first_str, 
                "/root/sdcard/app/instrument_info/%s/battery_info_switch_record_%s.txt", 
                pRecordDateTime, pRecordDateTime);
            if(stat(info_first_str, &stBatteryInfoFile) == 0)
			{
                iBatteryInfoFileSize = stBatteryInfoFile.st_size;
            }
            trace(2, "[%s:%s:%d] /root/sdcard/app/instrument_info/%s/battery_info_switch_record_%s.txt = %d", 
                    __FILE__, __FUNCTION__, __LINE__, 
                    pRecordDateTime, pRecordDateTime, iBatteryInfoFileSize);
                    
            if((iBatteryInfoFileSize < INFO_STR_LEN) && (iBatteryInfoFileSize > 0))
			{
                sprintf(info_first_str, 
                    "cat /root/sdcard/app/instrument_info/%s/battery_info_switch_record_%s.txt", 
                    pRecordDateTime, pRecordDateTime);
	            get_cmd_printf(info_first_str, battery_info_string_ptr, USART_INFO_STR_LEN);
                // Remove last ",\r\n"
                battery_info_string_ptr[strlen(battery_info_string_ptr) - INFO_STR_LAST_COMMA] = '\0';
                trace(2, "[%s:%s:%d] battery_info_string_ptr = %s", 
                            __FILE__, __FUNCTION__, __LINE__, battery_info_string_ptr);
			}
            else
            {
                sprintf(battery_info_string_ptr, "\"%s 00:00:00\",0", pRecordDateTime);
                // Change "2024_07_22 ..." into "2024-07-22 ..."
                battery_info_string_ptr[5] = '-';
                battery_info_string_ptr[8] = '-';
            }
            snprintf(info_str, INFO_STR_LEN, "{  \"BatteryInfo\": [ %s ] }", battery_info_string_ptr);
            
            trace(2, "[%s:%s:%d] info_str = %s", __FILE__, __FUNCTION__, __LINE__, info_str);
            websSetStatus(wp, 200);
            websWriteHeaders(wp, -1, 0);
            websWriteEndHeaders(wp);
            websWrite(wp, info_str);
            websFlush(wp);
        }
        websDone(wp);
		free(battery_info_string_ptr);
    }
    else if(strcmp(pMode, "get_usart_info") == 0)
    {
        char *usart_info_string_ptr = (char *)malloc(USART_INFO_STR_LEN);
        pRecordDateTime = websGetVar(wp, "record_time", "");
        if(strlen(pRecordDateTime) > 0)
        {
            int iUsartInfoFileSize = 0;
            struct stat stUsartInfoFile;
            sprintf(info_first_str, 
                "/root/sdcard/app/instrument_info/%s/usart_info_switch_record_%s.txt", 
                pRecordDateTime, pRecordDateTime);
            if(stat(info_first_str, &stUsartInfoFile) == 0)
			{
                iUsartInfoFileSize = stUsartInfoFile.st_size;
            }
            trace(2, "[%s:%s:%d] /root/sdcard/app/instrument_info/%s/usart_info_switch_record_%s.txt = %d", 
                    __FILE__, __FUNCTION__, __LINE__, 
                    pRecordDateTime, pRecordDateTime, iUsartInfoFileSize);
                    
            if((iUsartInfoFileSize < INFO_STR_LEN) && (iUsartInfoFileSize > 0))
			{
                sprintf(info_first_str, 
                    "cat /root/sdcard/app/instrument_info/%s/usart_info_switch_record_%s.txt", 
                    pRecordDateTime, pRecordDateTime);
	            get_cmd_printf(info_first_str, usart_info_string_ptr, USART_INFO_STR_LEN);
                // Remove last ",\r\n"
                usart_info_string_ptr[strlen(usart_info_string_ptr) - INFO_STR_LAST_COMMA] = '\0';
                trace(2, "[%s:%s:%d] usart_info_string_ptr = %s", 
                            __FILE__, __FUNCTION__, __LINE__, usart_info_string_ptr);
			}
            else
            {
                sprintf(usart_info_string_ptr, "\"%s 00:00:00\",0", pRecordDateTime);
                // Change "2024_07_22 ..." into "2024-07-22 ..."
                usart_info_string_ptr[5] = '-';
                usart_info_string_ptr[8] = '-';
            }
            snprintf(info_str, INFO_STR_LEN, "{  \"BatteryInfo\": [ %s ] }", usart_info_string_ptr);
            
            trace(2, "[%s:%s:%d] info_str = %s", __FILE__, __FUNCTION__, __LINE__, info_str);
            websSetStatus(wp, 200);
            websWriteHeaders(wp, -1, 0);
            websWriteEndHeaders(wp);
            websWrite(wp, info_str);
            websFlush(wp);
        }
        websDone(wp);
		free(usart_info_string_ptr);
    }
    else
    {
        websDone(wp);
    }
    trace(2, "%ld - [%s:%s:%d] websWrite::info_str = %s",
                      time(NULL), __FILE__, __FUNCTION__, __LINE__, info_str);
}

void sumbitProc(Webs *wp)
{

}

int calc_align(int value , int align)
{
    return (value + align -1) & (~(align -1));
}

static unsigned short gen_sum(unsigned char key, char * file_pos, unsigned int filelen)
{
    unsigned char key1 = key;
    unsigned char key2 = key;

    unsigned int i;

    if (filelen == 0)
        return 0;

    for(i=0; i<filelen; i++) {
        if (file_pos[i] == (char)key) {
            key2 += 1;
        } else {
            key1 ^= file_pos[i];
        }
    }
    return (unsigned short)key1 + ((unsigned short)key2 << 8);
}

int decode_firmware(char *cFileName, char *outputPath)
{
    int iFileLen = 0, iFileAlignLen = 0;
    struct stat st;

    char outFilePath[128];

    FIRMWARE_T firmware_head;

    if (lstat(cFileName, &st) < 0) {
        printf("lstat failed: %s\r\n", cFileName);
        return 0;
    }
    iFileLen = st.st_size;
    iFileAlignLen = calc_align(iFileLen, 1024);

    char *buf = NULL;
    buf = (char *)malloc(iFileAlignLen);
    memset(buf, 0, iFileAlignLen);

    FILE * fp = fopen(cFileName, "r");
    if(fp < 0)
    {
        free(buf);
        printf("open failed: %s\r\n", cFileName);
        return 0;
    }

    int n = fread(buf, sizeof(char), iFileLen, fp);

    memcpy(&firmware_head, buf, sizeof(FIRMWARE_T));
    if(firmware_head.magic != FIRMWARE_MAGIC)
    {
        free(buf);
        fclose(fp);
        printf("Error FIRMWARE_MAGIC\n");
        return 0;
    }
    // Check sysloader sum
    unsigned short uCheckSum = gen_sum(FIRMWARE_SYSLOADER_KEY,
                    buf + sizeof(FIRMWARE_T),
                    firmware_head.s_len);
    if(uCheckSum != firmware_head.s_sum)
    {
        free(buf);
        fclose(fp);
        printf("Error s_num\n");
        return 0;
    }

    // Check u-boot sum
    uCheckSum = gen_sum(FIRMWARE_UBOOT_KEY,
                    buf + sizeof(FIRMWARE_T) + firmware_head.s_len,
                    firmware_head.u_len);
    if(uCheckSum != firmware_head.u_sum)
    {
        free(buf);
        fclose(fp);
        printf("Error u_num\n");
        return 0;
    }

    // Check kernel sum
    uCheckSum = gen_sum(FIRMWARE_KERNEL_KEY,
                    buf + sizeof(FIRMWARE_T) + firmware_head.s_len + firmware_head.u_len,
                    firmware_head.k_len);
    if(uCheckSum != firmware_head.k_sum)
    {
        free(buf);
        fclose(fp);
        printf("Error k_num\n");
        return 0;
    }

    // Check rootfs sum
    uCheckSum = gen_sum(FIRMWARE_ROOTFS_KEY,
                    buf + sizeof(FIRMWARE_T) + firmware_head.s_len + firmware_head.u_len + firmware_head.k_len,
                    firmware_head.r_len);
    if(uCheckSum != firmware_head.r_sum)
    {
        free(buf);
        fclose(fp);
        printf("Error r_num\n");
        return 0;
    }

    if(firmware_head.s_len > 0)
    {
        // unpacked sysloader
        sprintf(outFilePath, "%s/%s", outputPath, FIRMWARE_SYSLOADER_FILENAME);
        FILE * fpSysloader = fopen(outFilePath, "w");
        if(fpSysloader < 0)
        {
            free(buf);
            fclose(fp);
            printf("open failed: %s\r\n", FIRMWARE_SYSLOADER_FILENAME);
            return 0;
        }
        fwrite(buf + sizeof(FIRMWARE_T), sizeof(char), firmware_head.s_len, fpSysloader);
        fclose(fpSysloader);
        printf("Unpack success: %s\r\n", FIRMWARE_SYSLOADER_FILENAME);
    }

    if(firmware_head.u_len > 0)
    {
        // unpacked u-boot
        sprintf(outFilePath, "%s/%s", outputPath, FIRMWARE_UBOOT_FILENAME);
        FILE * fpUboot = fopen(outFilePath, "w");
        if(fp < 0)
        {
            free(buf);
            fclose(fp);
            printf("open failed: %s\r\n", FIRMWARE_UBOOT_FILENAME);
            return 0;
        }
        fwrite(buf + sizeof(FIRMWARE_T) + firmware_head.s_len, sizeof(char), firmware_head.u_len, fpUboot);
        fclose(fpUboot);
        printf("Unpack success: %s\r\n", FIRMWARE_UBOOT_FILENAME);
    }

    if(firmware_head.k_len > 0)
    {
        // unpacked kernel
        sprintf(outFilePath, "%s/%s", outputPath, FIRMWARE_KERNEL_FILENAME);
        FILE * fpKernel = fopen(outFilePath, "w");
        if(fpKernel < 0)
        {
            free(buf);
            fclose(fp);
            printf("open failed: %s\r\n", FIRMWARE_KERNEL_FILENAME);
            return 0;
        }
        fwrite(buf + sizeof(FIRMWARE_T) + firmware_head.s_len + firmware_head.u_len,
                    sizeof(char), firmware_head.k_len, fpKernel);
        fclose(fpKernel);
        printf("Unpack success: %s\r\n", FIRMWARE_KERNEL_FILENAME);
    }

    if(firmware_head.r_len > 0)
    {
        // unpacked rootfs
        sprintf(outFilePath, "%s/%s", outputPath, FIRMWARE_ROOTFS_FILENAME);
        FILE * fpRootfs = fopen(outFilePath, "w");
        if(fpRootfs < 0)
        {
            free(buf);
            fclose(fp);
            printf("open failed: %s\r\n", FIRMWARE_ROOTFS_FILENAME);
            return 0;
        }
        fwrite(buf + sizeof(FIRMWARE_T) + firmware_head.s_len + firmware_head.u_len + firmware_head.k_len,
                    sizeof(char), firmware_head.r_len, fpRootfs);
        fclose(fpRootfs);
        printf("Unpack success: %s\r\n", FIRMWARE_ROOTFS_FILENAME);
    }

    free(buf);
    fclose(fp);
    return 1;
}

int check_uploadfile(Webs *wp)
{
    char            key[64];

    char app_sab_str[APP_PATH_LEN] = "";
    char app_scode_str[APP_PATH_LEN] = "";
    char app_firmware_str[APP_PATH_LEN] = "";
    struct stat stat_buf;
    char *pPathVal;
    char *pFileNameVal;

    pPathVal = websGetVar(wp, "UPLOAD_DIR", "/data/upload");
    trace(2, "[%s:%s:%d] pVal = %s", __FILE__, __FUNCTION__, __LINE__, pPathVal);

    sprintf(app_sab_str, "%s/app.sab", pPathVal);
    sprintf(app_scode_str, "%s/app.scode", pPathVal);
    // Get filename
    fmt(key, sizeof(key), "FILE_CLIENT_FILENAME_%s", wp->uploadVar);
    pFileNameVal = websGetVar(wp, key, "");
    char *filename_start_pos = strstr(pFileNameVal, "firmware_V");
    if (filename_start_pos)
    {
        sprintf(app_firmware_str, "%s/%s", pPathVal, pFileNameVal);
    }

    if(stat(app_sab_str, &stat_buf)==0)
    {
        memset(app_sab_str, 0x00, APP_PATH_LEN);
        sprintf(app_sab_str, "mv %s/app.sab /root/app/", pPathVal);
        system(app_sab_str);
        system("chmod 664 /root/app/app.sab");
        // Stop svm and watch dog would restart svm
        system("/root/app/www/stop_svm.sh &");
        // system("/etc/rc.d/rc.svm &");
        trace(2, "[%s:%s:%d] uploadProc :: update by %s",
                        __FILE__, __FUNCTION__, __LINE__, app_sab_str);
    }
    else if(stat(app_scode_str, &stat_buf)==0)
    {
        memset(app_scode_str, 0x00, APP_PATH_LEN);
        sprintf(app_scode_str, "mv %s/app.scode /root/app/", pPathVal);
        system(app_scode_str);
        system("chmod 664 /root/app/app.scode");
        // Stop svm and watch dog would restart svm
        system("/root/app/www/stop_svm.sh &");
        // system("/etc/rc.d/rc.svm &");
        trace(2, "[%s:%s:%d] uploadProc :: update by %s",
                            __FILE__, __FUNCTION__, __LINE__, app_scode_str);
    }
    else if(stat(app_firmware_str, &stat_buf)==0)
    {
        int iRet = decode_firmware(app_firmware_str, pPathVal);
        if(iRet == 1)
        {
            remove(app_firmware_str);
            trace(2, "[%s:%s:%d] uploadProc :: update by %s",
                            __FILE__, __FUNCTION__, __LINE__, app_firmware_str);


        }
        trace(2, "[%s:%s:%d] uploadProc :: update %s error",
                            __FILE__, __FUNCTION__, __LINE__, app_firmware_str);
        return 1;
    }
    else
    {
        trace(2, "[%s:%s:%d] uploadProc :: update error",
                                    __FILE__, __FUNCTION__, __LINE__);
        return 1;
    }
    return 0;
}

void uploadProc(Webs *wp)
{
    char info_str[INFO_STR_LEN];
    int iRet = check_uploadfile(wp);
    sprintf(info_str, "%d", iRet);
    websSetStatus(wp, 200);
    websWriteHeaders(wp, -1, 0);
    websWriteEndHeaders(wp);
    websWrite(wp, info_str);
    websFlush(wp);
    websDone(wp);
}

void upgradeProc(Webs *wp)
{
    char info_str[INFO_STR_LEN];
    int iRet = check_uploadfile(wp);
    sprintf(info_str, "%d", iRet);
    websSetStatus(wp, 200);
    websWriteHeaders(wp, -1, 0);
    websWriteEndHeaders(wp);
    websWrite(wp, info_str);
    websFlush(wp);
    websDone(wp);
}

void UserDefineInitialize()
{
    websDefineAction("status", statusProc);

    websDefineAction("uploadProc", uploadProc);
    websDefineAction("upgrade", upgradeProc);
    websDefineAction("sumbitProc", sumbitProc);
}

void UserDefineDeinitialize()
{

}

