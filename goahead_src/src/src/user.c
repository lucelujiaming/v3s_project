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

#define    TIME_SPAN               1 //   15mins  120       // 2 minutes
// #define    TIME_SCALE              (24 * 60 * 60 / TIME_SPAN)
#define    HOUR_SCALE              (60 * 60 / TIME_SPAN)

// #define INSTRUMENT_INFO_STR_LEN     0xC000
// INSTRUMENT_INFO_STR_LEN = HOUR_SCALE * 128
// 128 ie the length of following log:
// [-0.000821,-0.000821,-0.000821,-0.000821,-0.000821,-0.000821,-0.000821,-0.000821,-0.000821,-0.000821],
#define INSTRUMENT_INFO_STR_LEN     0x70800    
#define BATTERY_INFO_STR_LEN        1024
#define USART_INFO_STR_LEN          1024
// ulimit -s is 8192(KB) = 0x800000
#define INFO_STR_LEN                (INSTRUMENT_INFO_STR_LEN + BATTERY_INFO_STR_LEN + USART_INFO_STR_LEN) // 5120

#define INFO_STR_LAST_COMMA         2

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

int is_current_date(char * recordDateTime) {
    char strDate[128] = {0};	
	time_t timeNow = time(NULL);
	struct tm*	   tmNow	= localtime(&timeNow);
	
    sprintf(strDate, "%04d_%02d_%02d", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
	if(strncmp(strDate, recordDateTime, strlen(strDate)) == 0)
	{
		return 1;   // recordDateTime is today.
	}
	else {
		return 0;  // recordDateTime is not today.
	}
}

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

int getSequenceNumber(char * sDataLine, char * strPreFix)
{
    char cStartMinute[4] = {0};
    char cStartSecond[4] = {0};
    if(strlen(sDataLine) > (strlen(strPreFix) + 6))
    {
        // Calculate Sequence Number
        memcpy(cStartMinute, sDataLine + strlen(strPreFix) + 1, 2);
        int iStartMinute = atoi(cStartMinute);
        memcpy(cStartSecond, sDataLine + strlen(strPreFix) + 4, 2);
        int iStartSecond = atoi(cStartSecond);
        
        if(iStartMinute >= 0 || iStartSecond >= 0)
        { 
            return iStartMinute * 60 + iStartSecond;
        }
    }
    printf("Error sDataLine = %s, strPreFix = %s. \r\n", sDataLine, strPreFix);
    return -1;
}

int truncateLoglineWithChannelNum(char * sDataLine, int iChannNum)
{
    char * cTruncatePos = sDataLine;
    // printf("[%s:%s:%d] Enter sDataLine = %s, iChannNum = %d. \r\n", 
	//			__FILE__, __FUNCTION__, __LINE__, sDataLine, iChannNum);
    for(int i = 0; i < iChannNum; i++)
    {
        cTruncatePos = strchr(cTruncatePos, ',');
        if(cTruncatePos)
        {
            cTruncatePos++;
        }
        else {
            printf("[%s:%s:%d] Error cTruncatePos. \r\n", 
				__FILE__, __FUNCTION__, __LINE__);
            
        }
    }
    if(cTruncatePos)
    {
        // Omit last ','
        cTruncatePos--;
        cTruncatePos[0] = ']';
        cTruncatePos[1] = ',';
        cTruncatePos[2] = '\r';
        cTruncatePos[3] = '\n';
        cTruncatePos[4] = '\0';
    }
    // printf("[%s:%s:%d] Left sDataLine = %s, iChannNum = %d. \r\n", 
	//			__FILE__, __FUNCTION__, __LINE__, sDataLine, iChannNum);
}

char * createEmptyRecordWithChannelNum(char * cEmptyRecord, int iLen, int iChannNum)
{
	memset(cEmptyRecord, 0x00, iLen);
    // Create cEmptyRecord and cFillRecord by iChannNum
    strcat(cEmptyRecord, "[");
    for(int i = 0; i < iChannNum - 1; i++)
    {
        strcat(cEmptyRecord, "0.0,");
    }
    strcat(cEmptyRecord, "0.0] \r\n");
	return cEmptyRecord;
}

char * findLastPosOfPreFix(char * sFileDataLine, char * strPreFix)
{
    char* cPosOfPreFixPtr = NULL;
    char* cLastPosOfPreFixPtr = NULL;
    cPosOfPreFixPtr = strstr(sFileDataLine, strPreFix);
    // printf("[%s:%s:%d] sFileDataLine = %s.\r\n",
    //                     __FILE__, __FUNCTION__, __LINE__, sFileDataLine);
    // printf("[%s:%s:%d] cPosOfPreFixPtr = %s.\r\n",
    //                    __FILE__, __FUNCTION__, __LINE__, cPosOfPreFixPtr);
    while(cPosOfPreFixPtr != NULL)
    {
        cLastPosOfPreFixPtr = cPosOfPreFixPtr;
        cPosOfPreFixPtr = strstr(cPosOfPreFixPtr + strlen(strPreFix), strPreFix);
    }
    return cLastPosOfPreFixPtr;
}
/***********************************************************************
 * 函数名：    get_hourdata_from_logfile
 * 入口参数：  iYear/iMonth/iDay/iHour: 年月日和小时。
 *              strOutput:      对应年月日和小时的日志。
 * 返回值：     成功返回  数据条数。条数不应该大于3600条。
 *              否则返回   -1
 ***********************************************************************/
int get_hourdata_from_logfile(int iYear, int iMonth, int iDay, int iHour, 
								int iChannNum, char* strOutput, int iStrOutputLen, const char* filename)
{
    // 查找标志。在找到对应年月日和小时的日志的时候置为一。
    int iFound = -1;
    // 日志文件中每一行的序号，也就是当前这一行和传入的
    // 年月日和小时所差的秒数。从零开始。最大为3599。
    int iTotalSecond = 0;
    // 当前获取的日志条数。
    // 这个值应该时刻跟随iTotalSecond，相差不能超过一。否则就说明日志有孔洞。
    // 程序用这两个值来判断日志是否有孔洞。这里的孔洞包括两种情况。
    // 1. 第一种情况是非整点启动。例如你的程序是10点15分启动的。
    //    那么从10点00分到10点15分的日志就是一个孔洞。
    // 2. 第一种情况是中途程序关闭。例如一个程序在10点15分启动的。
    //    在11点15分的时候关闭。之后在11点30分的时候，再次启动。
    //    那么从11点15分到11点30分的日志就是一个孔洞。
    // 随便说一句，尾部的数据缺失不会当做孔洞。
    // 例如一个程序在10点40分的时候关闭。之后在11点20分的时候，再次启动。
    // 如果想获得10点的日志，程序只需要返回10点00分到10点40分的日志即可。
    // 不需要补齐从10点40分到10点59分的日志。这部分会由网页自动补零。
    int iLineCount        = 0;
    int iFillOutputLen    = 0;
	
    char * cTimeStampExampleTemplate = "1979-01-01 01:01:00,";
	// char * cEmptyRecordTemplate = "[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0] \r\n";
	// char * cFillRecordTemplate  = "[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0],\r\n";
    
    char cEmptyRecord[128]      = {0};
    char cFillRecord[128] = {0};
    
    char sFileDataLine[1024];
    char strPreFix[32] = {0};
    char * sDataLinePtr = NULL;
    
    FILE* fpr;
    if (NULL == (fpr = fopen(filename, "r")))
    {
		printf("logfile read %s failed\n", filename);
		return -1;// 读取原文件
    }
    sprintf(strPreFix, "%04d-%02d-%02d %02d", iYear, iMonth, iDay, iHour);
    printf("strPreFix = %s. \r\n", strPreFix);
    
    if(iChannNum <= 0)
    {
		printf("iChannNum is %d and <= 0\n", iChannNum);
		return -1;// 读取原文件
    }
    // Create cEmptyRecord and cFillRecord by iChannNum
    strcat(cEmptyRecord, "[");
    strcat(cFillRecord, "[");
    for(int i = 0; i < iChannNum - 1; i++)
    {
        strcat(cEmptyRecord, "0.0,");
        strcat(cFillRecord, "0.0,");
    }
    strcat(cEmptyRecord, "0.0] \r\n");
    strcat(cFillRecord, "0.0],\r\n");
    
    // Get log data
    while (NULL != fgets(sFileDataLine, 1024, fpr)) {
        if(iLineCount > 3600)
        {
            printf("[%s:%s:%d] sFileDataLine = %s, strPreFix = %s iLineCount = %d. \r\n",
                        __FILE__, __FUNCTION__, __LINE__, sFileDataLine, strPreFix, iLineCount);
            printf("iLineCount > 3600. Please Check log file. \r\n");
            return -1;
        }
        else
        {
        //    printf("[%s:%s:%d] sFileDataLine = %s, strPreFix = %s iLineCount = %d. \r\n",
        //                __FILE__, __FUNCTION__, __LINE__, sFileDataLine, strPreFix, iLineCount);
            ;
        }
        // 找到前缀标示的起始位置
        // printf("sFileDataLine = %s, strPreFix = %s. \r\n", sFileDataLine, strPreFix);
        sDataLinePtr = findLastPosOfPreFix(sFileDataLine, strPreFix);
        if(sDataLinePtr == NULL)
        {
            if (0 == iFound) {
                printf("[%s:%s:%d] 前缀标示的数据已经结束. \r\n", 
                                            __FILE__, __FUNCTION__, __LINE__);
                break;
            }
            else
            {
                continue;
            }
        }
        if (0 == strncmp(strPreFix, sDataLinePtr, strlen(strPreFix))) {
            iFound = 0;
            // 计算每一行中时间戳的序号，从0开始。
            iTotalSecond = getSequenceNumber(sDataLinePtr, strPreFix);
            if(iTotalSecond == -1)
            {
                break;
            }
            // 如果序号错位，表明出现孔洞。则需要补点。
            if(iTotalSecond > iLineCount){
                iFillOutputLen = strlen(cFillRecord) * (iTotalSecond - iLineCount);
                if(iFillOutputLen > iStrOutputLen)
                {
                    printf("Fill string is larger than input buffer length. \r\n");
                    break;
                }
                // We found a hole in the log
                for(int i = iLineCount; i < iTotalSecond; i++)
                {
                    strcat(strOutput, cFillRecord);
                }
                iLineCount = iTotalSecond;
            }
            // Truncate sDataLine
            truncateLoglineWithChannelNum(sDataLinePtr + strlen(cTimeStampExampleTemplate), iChannNum);
            // iFillOutputLen = strlen(strOutput) + strlen(sDataLinePtr) - strlen(cTimeStampExampleTemplate);
            iFillOutputLen = iFillOutputLen + strlen(sDataLinePtr) - strlen(cTimeStampExampleTemplate);
            // printf("iFillOutputLen = %d, iStrOutputLen = %d. \r\n", iFillOutputLen, iStrOutputLen);
            if(iFillOutputLen > iStrOutputLen)
            {
                printf("Record string is larger than input buffer length. \r\n");
                break;
            }
            
            strcat(strOutput, sDataLinePtr + strlen(cTimeStampExampleTemplate));
            iLineCount++;
        }
        else if (0 == iFound) { // 前缀标示的数据已经结束。
            printf("[%s:%s:%d] 前缀标示的数据已经结束. \r\n", 
                                        __FILE__, __FUNCTION__, __LINE__);
            break;  
        }
    }
    fclose(fpr);
    // 如果整个小时的日志都不存在。
    if(iLineCount == 0)
    {
        printf("整个小时的日志都不存在. \r\n");
        strcat(strOutput, cEmptyRecord);
        iLineCount = 1;
    }
    return iLineCount;
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
    char * pSeq = NULL;
    char *pRecordDate = NULL, *pRecordTime = NULL;
    char *ret_str;
    char *pMode;
    char *pValue;
    int   iValue;
    char info_first_str[256] = { 0 };
    char info_second_str[256] = { 0 };
    char info_str[INFO_STR_LEN] = { 0 };

    // char *pStartDateTime, *pEndDateTime;

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

        get_cmd_printf("cat /root/app/board_cpuloadinfo.txt", cpuload_string, APP_PATH_LEN);
        get_cmd_printf("cat /root/app/board_meminfo.txt", mem_string, APP_PATH_LEN);
        get_cmd_printf("cat /root/app/svm_info.txt", svm_string, APP_PATH_LEN);

        sprintf(info_str, "{ \"svm\": \"%s\", \"cpuload\": \"%s\", \"mem\": \"%s\" }",
           svm_string, cpuload_string, mem_string);
        websSetStatus(wp, 200);
        websWriteHeaders(wp, -1, 0);
        websWriteEndHeaders(wp);
        websWrite(wp, info_str);
        websFlush(wp);
        websDone(wp);
    }
    else if(strcmp(pMode, "get_instrument_config") == 0)
    {
		// Call ptc310_config_editor
        get_cmd_printf("/root/app/ptc310_config_editor GETMBJ", info_str, INFO_STR_LEN);
		
        websSetStatus(wp, 200);
        websWriteHeaders(wp, -1, 0);
        websWriteEndHeaders(wp);
        websWrite(wp, info_str);
        websFlush(wp);
        websDone(wp);
    }
    else if(strcmp(pMode, "set_instrument_config") == 0)
    {
    	char * pInstType       = websGetVar(wp, "inst_type", "");
    	char * pInstAddr       = websGetVar(wp, "inst_addr", "");
    	char * pInstTimeSpan   = websGetVar(wp, "inst_timespan", "");
    	char * pInstTimeOut    = websGetVar(wp, "inst_timeout", "");
    	char * pInstEndian     = websGetVar(wp, "inst_endian", "");
    	char * pInstFaultTimes = websGetVar(wp, "inst_fault_times", "");

		if(strlen(pInstType) > 0 && strlen(pInstAddr) > 0 
			&& strlen(pInstTimeSpan) > 0 && strlen(pInstTimeOut) > 0
			&& strlen(pInstEndian) > 0 && strlen(pInstFaultTimes) > 0)
		{
        	sprintf(info_first_str, 
				"/root/app/ptc310_config_editor SETMB %s %s %s %s %s %s",
				pInstType, pInstAddr, pInstTimeSpan, 
				pInstTimeOut, pInstEndian, pInstFaultTimes);
			trace(2, "[%s:%s:%d] info_first_str = %s", 
				__FILE__, __FUNCTION__, __LINE__, info_first_str);
			get_cmd_printf(info_first_str, info_str, INFO_STR_LEN);
		}
		
        websSetStatus(wp, 200);
        websWriteHeaders(wp, -1, 0);
        websWriteEndHeaders(wp);
        websWrite(wp, info_str);
        websFlush(wp);
		
        websDone(wp);
    }
    else if(strcmp(pMode, "get_latest_reading") == 0)
    {
		// Call ptc310_config_editor
        get_cmd_printf("/root/app/ptc310_config_editor GETLV", info_str, INFO_STR_LEN);
		
        websSetStatus(wp, 200);
        websWriteHeaders(wp, -1, 0);
        websWriteEndHeaders(wp);
        websWrite(wp, info_str);
        websFlush(wp);
        websDone(wp);
    }
    else if(strcmp(pMode, "get_history_datelist") == 0)
    {
		// 4096 / 15 = 273 days
        char datelist_string[4096] = "";
        sprintf(info_first_str, 
           "ls -d /root/sdcard/app/instrument_info/2*_*_* | sed 's/\\\///' | sed 's/.*info\\\///' | sed 's/\\\(.*\\\)/ \\\"\\1\\\",/'");
        trace(2, "[%s:%s:%d] info_first_str = %s", __FILE__, __FUNCTION__, __LINE__, info_first_str);
        get_cmd_printf(info_first_str, datelist_string, 4096);
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

        pRecordDate = websGetVar(wp, "record_date", "");
		pRecordTime = websGetVar(wp, "record_time", "");
        if(strlen(pRecordDate) > 0)
        {
			// The Directionary should always exists 
			// because record_time comes from get_history_datelist command.
			if(is_current_date(pRecordDate) == 1)
			{
		    	char cFilePathCommand[128] = {0};
		    	char cCopyFileOutput[128] = {0};
				// Copy current log to sdcard
				sprintf(cFilePathCommand, 
					"cp /root/app/instrument_info/* /root/sdcard/app/instrument_info/%s/ 2>&1",
					pRecordDate);
				get_cmd_printf(cFilePathCommand, cCopyFileOutput, 128);
			}

            int iInstrumentInfoFileSize = 0;
            struct stat stInstrumentInfoFile;
            sprintf(info_first_str, 
                "/root/sdcard/app/instrument_info/%s/instrument_history_record_%s.txt", 
                pRecordDate, pRecordDate);
            if(stat(info_first_str, &stInstrumentInfoFile) == 0)
			{
                iInstrumentInfoFileSize = stInstrumentInfoFile.st_size;
            }
            // trace(2, "[%s:%s:%d] /root/sdcard/app/instrument_info/%s/instrument_history_info_record_unixtime_%s.txt = %d", 
            //         __FILE__, __FUNCTION__, __LINE__, 
            //         pRecordDate, pRecordDate, iInstrumentInfoFileSize);
			
            int iUsartInfoFileSize = 0;
            struct stat stUsartInfoFile;
            sprintf(info_first_str, 
                "/root/sdcard/app/instrument_info/%s/usart_info_record_unixtime_%s.txt", 
                pRecordDate, pRecordDate);
            if(stat(info_first_str, &stUsartInfoFile) == 0)
			{
                iUsartInfoFileSize = stUsartInfoFile.st_size;
            }
            // trace(2, "[%s:%s:%d] /root/sdcard/app/instrument_info/%s/usart_info_record_unixtime_%s.txt = %d", 
            //         __FILE__, __FUNCTION__, __LINE__, 
            //         pRecordDate, pRecordDate, iUsartInfoFileSize);
			
            int iBatteryInfoFileSize = 0;
            struct stat stBatteryInfoFile;
            sprintf(info_first_str, 
                "/root/sdcard/app/instrument_info/%s/battery_info_record_unixtime_%s.txt", 
                pRecordDate, pRecordDate);
            if(stat(info_first_str, &stBatteryInfoFile) == 0)
			{
                iBatteryInfoFileSize = stBatteryInfoFile.st_size;
            }
            // trace(2, "[%s:%s:%d] /root/sdcard/app/instrument_info/%s/battery_info_record_unixtime_%s.txt = %d", 
            //        __FILE__, __FUNCTION__, __LINE__, 
            //         pRecordDate, pRecordDate, iBatteryInfoFileSize);

			int iRecordTime = atoi(pRecordTime);
			int iChannNum   = 0;
			char cChannNum[4] = {0};
			// Call ptc310_config_editor get channel number
	        get_cmd_printf("/root/app/ptc310_config_editor GETCHAN", cChannNum, 4);
			iChannNum = atoi(cChannNum);
			if((iInstrumentInfoFileSize > 0) && (iRecordTime > 0) 
				&& (strlen(pRecordDate) == strlen("1979_01_01")))
			{
				char cYear[8] = {0}, cMonth[4] = {0}, cDay[4] = {0}; 
				memcpy(cYear, pRecordDate, 4);
				memcpy(cMonth, pRecordDate + 5, 2);
				memcpy(cDay, pRecordDate + 8, 2);
                sprintf(info_first_str, "/root/sdcard/app/instrument_info/%s/instrument_history_record_%s.txt", 
                     pRecordDate, pRecordDate);
				memset(instrument_info_string_ptr, 0x00, INSTRUMENT_INFO_STR_LEN);
                trace(2, "[%s:%s:%d] get Year/Month/Day/Hour = '%d/%d/%d/%d/' from %s", __FILE__, __FUNCTION__, __LINE__, 
					atoi(cYear), atoi(cMonth), atoi(cDay), iRecordTime - 1, info_first_str);
				// When iRecordTime is 1, it means we get log from 00:00-01:00
				// When iRecordTime is 2, it means we get log from 01:00-02:00
				// So we use iRecordTime - 1 to get log
				get_hourdata_from_logfile(atoi(cYear), atoi(cMonth), atoi(cDay), iRecordTime - 1, 
							iChannNum, instrument_info_string_ptr, INSTRUMENT_INFO_STR_LEN, info_first_str);
                // trace(2, "[%s:%s:%d] instrument_info_string_ptr = '%s'", __FILE__, __FUNCTION__, __LINE__, instrument_info_string_ptr);
	            // get_cmd_printf(info_first_str, instrument_info_string_ptr, INSTRUMENT_INFO_STR_LEN);
                // Remove last ",\r\n"
                instrument_info_string_ptr[strlen(instrument_info_string_ptr) - INFO_STR_LAST_COMMA - 1] = '\0';
                // trace(2, "[%s:%s:%d] instrument_info_string_ptr = '%s'", __FILE__, __FUNCTION__, __LINE__, instrument_info_string_ptr);
			}
			else 
            {
				trace(2, "[%s:%s:%d] iInstrumentInfoFileSize = %d and iRecordTime = %d", 
				                    __FILE__, __FUNCTION__, __LINE__, 
				                    iInstrumentInfoFileSize, iRecordTime);
                // sprintf(instrument_info_string_ptr, "[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0]");
                sprintf(instrument_info_string_ptr, 
                    createEmptyRecordWithChannelNum(instrument_info_string_ptr, INSTRUMENT_INFO_STR_LEN, iChannNum));
                
            }
			
			if(iBatteryInfoFileSize > 0)
			{
                sprintf(info_first_str, 
                    "head -%d /root/sdcard/app/instrument_info/%s/battery_info_record_unixtime_%s.txt", 
                    HOUR_SCALE, pRecordDate, pRecordDate);
	            get_cmd_printf(info_first_str, battery_info_string_ptr, BATTERY_INFO_STR_LEN);
                // Remove last ",\r\n"
                battery_info_string_ptr[strlen(battery_info_string_ptr) - INFO_STR_LAST_COMMA] = '\0';
                trace(2, "[%s:%s:%d] battery_info_string_ptr = %s", __FILE__, __FUNCTION__, __LINE__, battery_info_string_ptr);
			}
			else 
            {
            	sprintf(battery_info_string_ptr, "0");
            }
			
			if(iUsartInfoFileSize > 0)
			{
                sprintf(info_first_str, 
                    "head -%d /root/sdcard/app/instrument_info/%s/usart_info_record_unixtime_%s.txt", 
                    HOUR_SCALE, pRecordDate, pRecordDate);
	            get_cmd_printf(info_first_str, usart_info_string_ptr, USART_INFO_STR_LEN);
                // Remove last ",\r\n"
                usart_info_string_ptr[strlen(usart_info_string_ptr) - INFO_STR_LAST_COMMA] = '\0';
			}
			else 
            {
            	sprintf(usart_info_string_ptr, "0");
            }

            snprintf(info_str, INFO_STR_LEN, 
                "{  \"Scale\": %d, \"ChannNum\": %d, \"InstrumentInfo\": [ %s ], \r\n\"BatteryInfo\": [ %s ], \r\n\"UsartInfo\": [ %s ] }", 
                HOUR_SCALE, iChannNum, instrument_info_string_ptr, battery_info_string_ptr, usart_info_string_ptr);
            
            trace(2, "[%s:%s:%d] info_str = %s", __FILE__, __FUNCTION__, __LINE__, info_str);
            trace(2, "[%s:%s:%d] strlen(info_str) = %d", __FILE__, __FUNCTION__, __LINE__, strlen(info_str));
            websSetStatus(wp, 200);
            websWriteHeaders(wp, -1, 0);
            websWriteEndHeaders(wp);
            websWrite(wp, info_str);
            websFlush(wp);
        }
        websDone(wp);
        trace(2, "[%s:%s:%d] websFlush end", __FILE__, __FUNCTION__, __LINE__);
		free(instrument_info_string_ptr);
		free(battery_info_string_ptr);
		free(usart_info_string_ptr);
    }
    else if(strcmp(pMode, "get_battery_info") == 0)
    {
        char *battery_info_string_ptr = (char *)malloc(BATTERY_INFO_STR_LEN);
        pRecordDate = websGetVar(wp, "record_date", "");
        trace(2, "[%s:%s:%d] %ld - websWrite::pRecordDate = %s",
                      __FILE__, __FUNCTION__, __LINE__, time(NULL), pRecordDate);
        if(strlen(pRecordDate) > 0)
        {
            int iBatteryInfoFileSize = 0;
            struct stat stBatteryInfoFile;
            sprintf(info_first_str, 
                "/root/sdcard/app/instrument_info/%s/battery_info_switch_record_%s.txt", 
                pRecordDate, pRecordDate);
            if(stat(info_first_str, &stBatteryInfoFile) == 0)
			{
                iBatteryInfoFileSize = stBatteryInfoFile.st_size;
            }
            trace(2, "[%s:%s:%d] /root/sdcard/app/instrument_info/%s/battery_info_switch_record_%s.txt = %d", 
                    __FILE__, __FUNCTION__, __LINE__, 
                    pRecordDate, pRecordDate, iBatteryInfoFileSize);
                    
            if((iBatteryInfoFileSize < INFO_STR_LEN) && (iBatteryInfoFileSize > 0))
			{
                sprintf(info_first_str, 
                    "cat /root/sdcard/app/instrument_info/%s/battery_info_switch_record_%s.txt", 
                    pRecordDate, pRecordDate);
	            get_cmd_printf(info_first_str, battery_info_string_ptr, USART_INFO_STR_LEN);
                // Remove last ",\r\n"
                battery_info_string_ptr[strlen(battery_info_string_ptr) - INFO_STR_LAST_COMMA] = '\0';
                trace(2, "[%s:%s:%d] battery_info_string_ptr = %s", 
                            __FILE__, __FUNCTION__, __LINE__, battery_info_string_ptr);
			}
            else
            {
                sprintf(battery_info_string_ptr, "\"%s 00:00:00\",0", pRecordDate);
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
        pRecordDate = websGetVar(wp, "record_date", "");
        if(strlen(pRecordDate) > 0)
        {
            int iUsartInfoFileSize = 0;
            struct stat stUsartInfoFile;
            sprintf(info_first_str, 
                "/root/sdcard/app/instrument_info/%s/usart_info_switch_record_%s.txt", 
                pRecordDate, pRecordDate);
            if(stat(info_first_str, &stUsartInfoFile) == 0)
			{
                iUsartInfoFileSize = stUsartInfoFile.st_size;
            }
            trace(2, "[%s:%s:%d] /root/sdcard/app/instrument_info/%s/usart_info_switch_record_%s.txt = %d", 
                    __FILE__, __FUNCTION__, __LINE__, 
                    pRecordDate, pRecordDate, iUsartInfoFileSize);
                    
            if((iUsartInfoFileSize < INFO_STR_LEN) && (iUsartInfoFileSize > 0))
			{
                sprintf(info_first_str, 
                    "cat /root/sdcard/app/instrument_info/%s/usart_info_switch_record_%s.txt", 
                    pRecordDate, pRecordDate);
	            get_cmd_printf(info_first_str, usart_info_string_ptr, USART_INFO_STR_LEN);
                // Remove last ",\r\n"
                usart_info_string_ptr[strlen(usart_info_string_ptr) - INFO_STR_LAST_COMMA] = '\0';
                trace(2, "[%s:%s:%d] usart_info_string_ptr = %s", 
                            __FILE__, __FUNCTION__, __LINE__, usart_info_string_ptr);
			}
            else
            {
                sprintf(usart_info_string_ptr, "\"%s 00:00:00\",0", pRecordDate);
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
    // trace(2, "%ld - [%s:%s:%d] websWrite::info_str = %s",
    //                  time(NULL), __FILE__, __FUNCTION__, __LINE__, info_str);
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

