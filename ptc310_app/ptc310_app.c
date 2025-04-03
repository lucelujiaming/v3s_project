#include <stdint.h>
#include <stdio.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <termios.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#include <sys/time.h>
#include <signal.h>
#include <time.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "param.h"

#include "modbus.h"
#include "usart.h"

#include "Protocol/protocol.h"
#include "Protocol/Modbus/ModbusSlave.h"

#include "modbus_rtu.h"
#include "config_operation.h"

#include "v3s_gpio_operation.h"

#include "v3s_udp_controller.h"

#define MODBUS_MAX_ADU_LENGTH  260


//从站地址 17
#define SERVER_ID 17

#define MODBUS_UART_DEVICE       "/dev/ttyS0"
#define INSTRUMENT_UART_DEVICE   "/dev/ttyS1"

// output_mix_history_trend
// 把时间间隔修改为一秒钟记录一次，分小时显示，每一次显示3600个点。
#define    INSTRUMENT_HISTORY_TIME_SPAN      1 //   15mins   600   // 10 minutes
#define    INSTRUMENT_HISTORY_HOUR_SCALE    (60 * 60 / INSTRUMENT_HISTORY_TIME_SPAN)
#define    INSTRUMENT_HISTORY_TIME_SCALE    (24 * 60 * 60 / INSTRUMENT_HISTORY_TIME_SPAN)
// #define    NEW_YEAR_DAY_2024    1704038400

char modbus_uart_device[20];
char instrument_uart_device[20];
// int  modbus_uart_response_interval = 20000;

// localtime()函数返回的是一个静态变量的指针，这个指针指向的内存是由C标准库管理的，属于静态存储区。
// 每次调用localtime()时，它都会返回一个指向同一个静态区域内存的指针，
// 这意味着多次调用localtime()会返回相同的指针，并且这个指针指向的内存会在程序结束时由系统自动释放‌。

// log_record_tm会在启动时设置为当天，并在日期变化的时候更新。
struct tm    log_record_tm;
// last_log_record_tm会在启动时设置为当天，并在日期变化的时候更新。
struct tm    last_log_record_tm;

PTC310_INTERFACE_STATUS g_iModbusUsartOfflineStatus = MODBUS_USART_ONLINE;
PTC310_INTERFACE_STATUS g_iBatteryOfflineStatus = BATTERY_ONLINE;


// 数据格式参见《PTC310_V2.7.3用户手册》
const uint16_t CP_DefaultValue[CP_EEP_MAX]= 
{
	// PT_MEECO,     // 基本寄存器： 40001 - 协议类型
	PT_DELTA_F,   // 基本寄存器： 40001 - 协议类型
	0,            // 基本寄存器： 40002 - 设备地址
	20,           // 基本寄存器： 40003 - 读取间隔(0.1s)
	5,            // 基本寄存器： 40004 - 超时时间(0.1s)
	0,            // 基本寄存器： 40005 - 高低字顺序
	3,            // 扩展寄存器： 40006 - 重试次数
	0,            // 扩展寄存器： 40007 - 保留位
	0             // 扩展寄存器： 40008 - 保留位
};
	

// output_mix_history_trend ends
static int get_cmd_printf(char *cmd, char *buf, int bufSize);
int append_logcontent_to_file(char * cFileName, char * cFileContent);


/***********************************************************************
 * 函数名：open_and_new_rtu_slave
 *   功能：打开Modbus协议的端口。
 * 入口参数： 用于保存原有配置信息的termios结构体对象。
 *   返回值： 返回端口文件句柄。
 ***********************************************************************/
int open_and_new_rtu_slave(struct termios* old_tios)
{
    char cDevicePath[20];
	int modbus_fd = 0;
    // 1. 打开Modbus口
	// 1.1. 设置串口信息
    if(strlen(modbus_uart_device) > 0)
    {
		memset(cDevicePath, 0x00, 20);
		sprintf(cDevicePath, "/dev/%s", modbus_uart_device);
		modbus_fd = modbus_rtu_connect(cDevicePath, 1200 * 16, 'N', 8, 1, old_tios);
    	printf("Modbus path uses %s...\n", cDevicePath);
    }
	else
    {
		modbus_fd = modbus_rtu_connect(MODBUS_UART_DEVICE, 1200 * 16, 'N', 8, 1, old_tios);
    	printf("Modbus default path is %s...\n", INSTRUMENT_UART_DEVICE);
	}
	if (-1 == modbus_fd)
	{
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return -1;
	}
	else
	{
		printf("设置串口信息成功\n");
	}
	return modbus_fd;
	
}

/***********************************************************************
 * 函数名：close_and_free_rtu_slave
 *   功能：关闭Modbus协议的端口。
 * 入口参数： 端口文件句柄和保存了原有配置信息的termios结构体对象。
 *   返回值： 关闭成功返回0，关闭失败返回-1。
 ***********************************************************************/

int close_and_free_rtu_slave(int modbus_fd, struct termios* old_tios)
{
    if (modbus_fd != -1) {
        tcsetattr(modbus_fd, TCSANOW, old_tios);
        close(modbus_fd);
        modbus_fd = -1;
    }
	return 0;
}

/***********************************************************************
 * 函数名：open_and_new_rtu_slave
 *   功能：打开仪表协议的端口。
 * 入口参数： 无。
 *   返回值： 返回端口文件句柄。
 ***********************************************************************/
int open_ptc_port()
{
    char cDevicePath[20];
	
    int   instrument_fd; // , send_res;
    // 2. 打开PTC私有协议对应的串口
    if(strlen(instrument_uart_device) > 0)
    {
		memset(cDevicePath, 0x00, 20);
		sprintf(cDevicePath, "/dev/%s", instrument_uart_device);
    	instrument_fd = open(cDevicePath, O_RDWR|O_NOCTTY/*|O_NDELAY*/);
    	printf("Instrument path uses %s...\n", cDevicePath);
    }
	else
    {
    	instrument_fd = open(INSTRUMENT_UART_DEVICE, O_RDWR|O_NOCTTY/*|O_NDELAY*/);
    	printf("Instrument default path is %s...\n", INSTRUMENT_UART_DEVICE);
    }
	printf("open_ptc_port: open return %d\n", instrument_fd);
    if (instrument_fd == -1) {
        perror("INSTRUMENT_UART_DEVICE open error");
        exit(1);
    }
	return instrument_fd;
}

/***********************************************************************
 * 函数名：out_instrument_reading_chart_history_record
 *   功能：记录一次最新仪表读数。
 * 入口参数： 记录仪表读数使用的时间戳，为当前时间。
 *   返回值： 无须返回。
 ***********************************************************************/
void out_instrument_reading_chart_history_record(time_t iFakeTimeStamp)
{
    char cFileName[128] = {0};
    char cFileContent[1024] = {0};
    char cProtocolDataOutput[1024] = {0};

    time_t timeNow;
    if(iFakeTimeStamp == 0)
    {
        timeNow = time(NULL);
    }
    else 
    {
        timeNow = iFakeTimeStamp;
    }
    struct tm*     tmNow    = localtime(&timeNow);

    // 1. 获得仪表读数。
	Protocol_DataOutput(cProtocolDataOutput);
	
	// printf("cProtocolDataOutput = [%s] at %04d-%02d-%02d %02d:%02d:%02d\r\n", 
	//		cProtocolDataOutput, 
    //        tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, 
	//		tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec);

	// 2. 为仪表读数添加时间。
    sprintf(cFileContent, "%04d-%02d-%02d %02d:%02d:%02d,[%s],\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, 
            tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec,
            cProtocolDataOutput);

    sprintf(cFileName, "instrument_reading_chart_history_record_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
	// 3. 记录到文件中。
    append_logcontent_to_file(cFileName, cFileContent);
	
	// 4. 为电池状态添加时间。
	memset(cFileContent, 0x00, 1024);
    sprintf(cFileContent, "%04d-%02d-%02d %02d:%02d:%02d,[%d],\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, 
            tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec,
            g_iBatteryOfflineStatus);
	memset(cFileName, 0x00, 128);
    sprintf(cFileName, "battery_usart_info_chart_history_record_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
	// 5. 记录到文件中。
    append_logcontent_to_file(cFileName, cFileContent);
	
	// 6. 为Modbus串口状态添加时间。
	memset(cFileContent, 0x00, 1024);
    sprintf(cFileContent, "%04d-%02d-%02d %02d:%02d:%02d,[%d],\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, 
            tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec,
            g_iModbusUsartOfflineStatus);
	memset(cFileName, 0x00, 128);
    sprintf(cFileName, "modbus_usart_info_chart_history_record_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
	// 7. 记录到文件中。
    append_logcontent_to_file(cFileName, cFileContent);
	
	// 8. 为仪表串口状态添加时间。
	memset(cFileContent, 0x00, 1024);
    sprintf(cFileContent, "%04d-%02d-%02d %02d:%02d:%02d,[%d],\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, 
            tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec,
            g_iInstrumentUsartOfflineStatus);
	memset(cFileName, 0x00, 128);
    sprintf(cFileName, "instrument_usart_info_chart_history_record_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
	// 9. 记录到文件中。
    append_logcontent_to_file(cFileName, cFileContent);
}

/***********************************************************************
 * 函数名：out_battery_info_record
 *   功能：记录一次电源状态。
 *         记录格式为状态变化时间和变化的状态。
 * 入口参数： 电源状态和记录电源状态使用的时间戳，为当前时间。
 *   返回值： 无须返回。
 ***********************************************************************/
void out_battery_info_record(PTC310_INTERFACE_STATUS iOfflineStatus, time_t iFakeTimeStamp)
{
    char cFileName[128];
    char cFileContent[1024];

    time_t timeNow;
    if(iFakeTimeStamp == 0)
    {
        timeNow = time(NULL);
    }
    else
    {
        timeNow = iFakeTimeStamp;
    }
    struct tm*     tmNow  = localtime(&timeNow);

    sprintf(cFileContent, "[\"%04d-%02d-%02d %02d:%02d:%02d\",%d],\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, 
            tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec, iOfflineStatus);

    sprintf(cFileName, "battery_info_record_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    append_logcontent_to_file(cFileName, cFileContent);    
}


/***********************************************************************
 * 函数名：check_battery_info_record
 *   功能：如果当天没有记录电源串口状态。
 *         记录一次。格式为状态变化时间和变化的状态。
 * 入口参数： 串口状态。
 *   返回值： 无须返回。
 ***********************************************************************/
void check_battery_info_record(PTC310_INTERFACE_STATUS iOfflineStatus)
{
	struct stat st;
    char cFileName[128];
    time_t timeNow;
	
    timeNow = time(NULL);
    struct tm*     tmNow  = localtime(&timeNow);
	
    sprintf(cFileName, "/root/app/instrument_info/battery_info_record_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);

    if (stat(cFileName, &st) == 0) {
        return ; // 存在
    } else {
    	printf("check_modbus_usart_info_record is %s...\n", cFileName);
        out_battery_info_record(iOfflineStatus, timeNow);
    }
}

/***********************************************************************
 * 函数名：out_modbus_usart_info_record
 *   功能：记录一次Modbus串口状态。
 *         记录格式为状态变化时间和变化的状态。
 * 入口参数： 串口状态和记录串口状态使用的时间戳，为当前时间。
 *   返回值： 无须返回。
 ***********************************************************************/
void out_modbus_usart_info_record(PTC310_INTERFACE_STATUS iOfflineStatus, time_t iFakeTimeStamp)
{
    char cFileName[128];
    char cFileContent[1024];

    time_t timeNow;
    if(iFakeTimeStamp == 0)
    {
        timeNow = time(NULL);
    }
    else 
    {
        timeNow = iFakeTimeStamp;
    }
    struct tm*     tmNow  = localtime(&timeNow);

    sprintf(cFileContent, "[\"%04d-%02d-%02d %02d:%02d:%02d\",%d],\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, 
            tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec, iOfflineStatus);

    sprintf(cFileName, "modbus_usart_info_record_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    append_logcontent_to_file(cFileName, cFileContent);
}

/***********************************************************************
 * 函数名：check_modbus_usart_info_record
 *   功能：如果当天没有记录Modbus串口状态。
 *         记录一次。格式为状态变化时间和变化的状态。
 * 入口参数： 串口状态。
 *   返回值： 无须返回。
 ***********************************************************************/
void check_modbus_usart_info_record(PTC310_INTERFACE_STATUS iOfflineStatus)
{
	struct stat st;
    char cFileName[128];
    time_t timeNow;
	
    timeNow = time(NULL);
    struct tm*     tmNow  = localtime(&timeNow);
	
    sprintf(cFileName, "/root/app/instrument_info/modbus_usart_info_record_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);

    if (stat(cFileName, &st) == 0) {
        return ; // 存在
    } else {
    	printf("check_modbus_usart_info_record is %s...\n", cFileName);
        out_modbus_usart_info_record(iOfflineStatus, timeNow);
    }
}

/***********************************************************************
 * 函数名：out_instrument_usart_info_record
 *   功能：记录一次instrument串口状态。
 *         记录格式为状态变化时间和变化的状态。
 * 入口参数： 串口状态和记录串口状态使用的时间戳，为当前时间。
 *   返回值： 无须返回。
 ***********************************************************************/
void out_instrument_usart_info_record(PTC310_INTERFACE_STATUS iOfflineStatus, time_t iFakeTimeStamp)
{
    char cFileName[128];
    char cFileContent[1024];

    time_t timeNow;
    if(iFakeTimeStamp == 0)
    {
        timeNow = time(NULL);
    }
    else 
    {
        timeNow = iFakeTimeStamp;
    }
    struct tm*     tmNow  = localtime(&timeNow);

    sprintf(cFileContent, "[\"%04d-%02d-%02d %02d:%02d:%02d\",%d],\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, 
            tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec, iOfflineStatus);

    sprintf(cFileName, "instrument_usart_info_record_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    append_logcontent_to_file(cFileName, cFileContent);
}

/***********************************************************************
 * 函数名：check_instrument_usart_info_record
 *   功能：如果当天没有记录instrument串口状态。
 *         记录一次。格式为状态变化时间和变化的状态。
 * 入口参数： 串口状态。
 *   返回值： 无须返回。
 ***********************************************************************/
void check_instrument_usart_info_record(PTC310_INTERFACE_STATUS iOfflineStatus)
{
	struct stat st;
    char cFileName[128];
    time_t timeNow;
	
    timeNow = time(NULL);
    struct tm*     tmNow  = localtime(&timeNow);
	
    sprintf(cFileName, "/root/app/instrument_info/instrument_usart_info_record_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);

    if (stat(cFileName, &st) == 0) {
        return ; // 存在
    } else {
    	printf("check_instrument_usart_info_record is %s...\n", cFileName);
        out_instrument_usart_info_record(iOfflineStatus, timeNow);
    }
}

// #define BATTERY_UART_DEVICE_TO        5
// #define BATTERY_UART_DEVICE        "/dev/ttyS0"
//	// int set_interface_attribs(int fd, int speed, int parity) {
//	int set_interface_attribs(int fd) {
//
//	    struct termios tty;
//	 
//	    if (tcgetattr(fd, &tty) != 0) {
//	        perror("tcgetattr");
//	        return -1;
//	    }
//	 
//	    // cfsetispeed(&tty, speed);
//	    // cfsetospeed(&tty, speed);
//	 
//	    // 设置超时
//	    tty.c_cc[VMIN]     = 0;     // 最小字符数
//	    tty.c_cc[VTIME]    = BATTERY_UART_DEVICE_TO;     // 超时时间（秒数，1秒为单位，5表示5秒超时）
//	 
//	    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
//	        perror("tcsetattr");
//	        return -1;
//	    }
//	 
//	    return 0;
//	}

//    int get_battery_status()
//    {
//        int   fd; // , send_res;
//        char cBatteryStatus[16];
//        
//        // 2. 打开PTC私有协议对应的串口
//        fd = open(BATTERY_UART_DEVICE, O_RDONLY);
//        set_interface_attribs(fd);
//        
//        read(fd, cBatteryStatus, 16);
//        close(fd);
//        if(strcmp(cBatteryStatus, "1") == 0)
//        {
//            return BATTERY_OFFLINE;
//        }
//        else 
//        {
//            return INSTRUMENT_ONLINE;
//        }
//    }

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
    {
		printf("cmd and bufis NULL\n");
        return -1;
    }

    if ((fp = popen(cmd, "r")) == NULL) {
		printf("popen cmd failed\n");
        return -1;
    }

    p = buffer;
    memset(buf, 0, bufSize);
    fgets(buffer, bufferSize, fp);
    do {
        writeLen = ((length + strlen(buffer)) > (bufSize - 1)) ? (bufSize - 1 - length) : strlen(buffer);

        memcpy(buf + length, buffer, writeLen);
        length += writeLen;
		// printf("get_cmd_printf length = %d\n", length);
    } while (fgets(buffer, bufferSize, fp) != NULL);


    if (*(p = &buf[length - 1]) == 0x0A)
    {
        *p = 0;
    }
	// printf("get_cmd_printf(%s) output <%s> with length = %d\n", cmd, buf, length);

    status = pclose(fp);
    if (WIFEXITED(status)) {

        return WEXITSTATUS(status);
    }

    return -1;
}

// 获得电池状态。
int get_battery_status()
{
    unsigned int uRet = V3S_GPIO_GetPin(V3S_PE, 0);
    
    if(uRet == 0)
    {
        return BATTERY_OFFLINE;
    }
    else 
    {
        return BATTERY_ONLINE;
    }
}

// 该文件目前未使用。
int append_file(char * cFileName, char * cFileContent)
{
	int iRet = 0;
    char cMkdirOutput[256];
    char cRemountOutput[256];
	
    char cFilePathWithName[128];
    char cFilePathMkdirCommand[128];
    int   append_fd; // , send_res;
    time_t timeNow = time(NULL);
    struct tm*     tmNow    = localtime(&timeNow);
	
    sprintf(cFilePathWithName, "/root/sdcard/app/instrument_info/%d_%02d_%02d/%s",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, cFileName);
    // 2. 打开PTC私有协议对应的串口
    append_fd = open(cFilePathWithName, O_RDWR | O_APPEND);
    if (append_fd == -1) {
		// 这里的2>&1表示将标准错误（文件描述符2）重定向到标准输出（文件描述符1）。
	    sprintf(cFilePathMkdirCommand, "mkdir -p /root/sdcard/app/instrument_info/%d_%02d_%02d/ 2>&1",
	            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
		memset(cMkdirOutput, 0x00, 256);
	    iRet = get_cmd_printf(cFilePathMkdirCommand, cMkdirOutput, 256);
	    printf("get_cmd_printf(%s) return %d\n",cFilePathMkdirCommand, iRet);
		if(strlen(cMkdirOutput) > 0)
		{
	        printf("Command <%s> error: errorInfo is %s\n", cFilePathMkdirCommand, cMkdirOutput);
			memset(cRemountOutput, 0x00, 256);
			get_cmd_printf("/root/app/www/remount_sdcard.sh", cRemountOutput, 256);
	        printf("remount_sdcard return %s\n", cRemountOutput);
		}
		
        append_fd = open(cFilePathWithName, O_RDWR | O_CREAT);
        if (append_fd == -1) {
	        printf("append_file: open failed return %d\n", append_fd);
            return -1;
        }
    }
	// printf("append_file: write %s return %d\n", cFileContent, append_fd);
	write(append_fd, cFileContent, strlen(cFileContent));
	close(append_fd);
    return 0;
}

#define   DIR_HAS_EXISTED      1
#define   DIR_NOT_EXISTS       0
int dir_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return DIR_HAS_EXISTED; // 目录存在
    } else {
        return DIR_NOT_EXISTS; // 目录不存在
    }
}


/********************************************************************
 * 这个函数用于处理SD卡概率性无法写入的问题。
 * 详细流程参见《SD卡无法写入问题的规避方法》。
 ********************************************************************/
#define   PRINT_MKDIR_OUTPUT_ON   1
#define   PRINT_MKDIR_OUTPUT_OFF  0
void sdcard_operation_process()
{
	static int isPrintMkdirOutput = PRINT_MKDIR_OUTPUT_ON;
	// int iRet = 0;
    char cMkdirOutput[256] = {0};
    // char cRemountOutput[256];
    
    char cFilePathCommand[128] = {0};
    time_t timeNow = time(NULL);
    struct tm*     tmNow    = localtime(&timeNow);
	
	/********************************************************************
	 * Step 2: 
 	 *   经过测试发现，在SD卡上频繁写入会导致SD卡无法创建目录。
 	 *   为了规避这个问题，需要构造一套逻辑来解决。
 	 *   方法是当天的日志写在内部存储上，每当日期变化，把之前的日志移动到SD卡上。
 	 *   这里首先把目标目录创建好。
	 ********************************************************************/
	sprintf(cFilePathCommand, "/root/sdcard/app/instrument_info/%d_%02d_%02d/",
	            log_record_tm.tm_year + 1900, log_record_tm.tm_mon + 1, log_record_tm.tm_mday);
	if(dir_exists(cFilePathCommand) == DIR_NOT_EXISTS)
	{
		memset(cFilePathCommand, 0x00, 128);
		// 这里的2>&1表示将标准错误（文件描述符2）重定向到标准输出（文件描述符1）。
	    sprintf(cFilePathCommand, "mkdir -p /root/sdcard/app/instrument_info/%d_%02d_%02d/ 2>&1",
	            log_record_tm.tm_year + 1900, log_record_tm.tm_mon + 1, log_record_tm.tm_mday);
		memset(cMkdirOutput, 0x00, 256);
	    // iRet = 
		get_cmd_printf(cFilePathCommand, cMkdirOutput, 256);
		printf("Exec command : <%s> return '%s'\n", cFilePathCommand, cMkdirOutput);
		if(strlen(cMkdirOutput) > 0)
		{
			if(isPrintMkdirOutput == PRINT_MKDIR_OUTPUT_ON)
			{
				printf("Command <%s> error: errorInfo is %s\n", cFilePathCommand, cMkdirOutput);
				isPrintMkdirOutput = PRINT_MKDIR_OUTPUT_OFF;
		        memset(cMkdirOutput, 0x00, 256);
                get_cmd_printf("/root/app/www/remount_sdcard.sh", cMkdirOutput, 256);
				printf("Exec command : <%s> return '%s'\n", cFilePathCommand, cMkdirOutput);
				if(strlen(cMkdirOutput) > 0)
				{
				   printf("Command remount_sdcard.sh error: errorInfo is %s\n", cMkdirOutput);
			    }
				else 
				{
				   printf("Command remount_sdcard.sh OK.\n");
			    }
			}
    	    memcpy(&log_record_tm, localtime(&timeNow), sizeof(struct tm));
	        // return -1;
		}
		else 
		{
			/********************************************************************
			 * Step 3: 
			 *   We start a new day and we need copy yesterday's log to SDCard.
			 ********************************************************************/
			memset(cFilePathCommand, 0x00, 128);
			sprintf(cFilePathCommand, "/root/sdcard/app/instrument_info/%d_%02d_%02d/",
			            last_log_record_tm.tm_year + 1900, last_log_record_tm.tm_mon + 1, last_log_record_tm.tm_mday);
			if(dir_exists(cFilePathCommand) == DIR_HAS_EXISTED)
			{
				memset(cFilePathCommand, 0x00, 128);
				sprintf(cFilePathCommand, 
					"cp /root/app/instrument_info/*_%d_%02d_%02d.txt /root/sdcard/app/instrument_info/%d_%02d_%02d/ 2>&1",
					last_log_record_tm.tm_year + 1900, last_log_record_tm.tm_mon + 1, last_log_record_tm.tm_mday,
					last_log_record_tm.tm_year + 1900, last_log_record_tm.tm_mon + 1, last_log_record_tm.tm_mday);
				memset(cMkdirOutput, 0x00, 256);
				get_cmd_printf(cFilePathCommand, cMkdirOutput, 256);
				printf("Exec command : <%s> return '%s'\n", cFilePathCommand, cMkdirOutput);
				if(strlen(cMkdirOutput) > 0)
				{
				    printf("Command <%s> error: errorInfo is %s\n", cFilePathCommand, cMkdirOutput);
				}
				else {
					/********************************************************************
					 * Step 4: 
					 *   We start a new day and we have copied yesterday's log to SDCard.
					 *   Now we can delete yesterday's log.
					 ********************************************************************/
					memset(cFilePathCommand, 0x00, 128);
					sprintf(cFilePathCommand, "rm /root/app/instrument_info/*_%d_%02d_%02d.txt 2>&1",
						last_log_record_tm.tm_year + 1900, last_log_record_tm.tm_mon + 1, last_log_record_tm.tm_mday);
					memset(cMkdirOutput, 0x00, 256);
					get_cmd_printf(cFilePathCommand, cMkdirOutput, 256);
					printf("Exec command : <%s> return '%s'\n", cFilePathCommand, cMkdirOutput);
					if(strlen(cMkdirOutput) > 0)
					{
					    printf("Command <%s> error: errorInfo is %s\n", cFilePathCommand, cMkdirOutput);
					}
	    	    	memcpy(&last_log_record_tm, localtime(&timeNow), sizeof(struct tm));
				}
			}
			else {
				printf("(%s) does not exist.\n",cFilePathCommand);
			}
		}
	}

	/********************************************************************
	 * Step 1: 
	 *   We start a new day and we set the flag of new day.
	 ********************************************************************/
	if((tmNow->tm_year != log_record_tm.tm_year)
		|| (tmNow->tm_mon != log_record_tm.tm_mon)
		|| (tmNow->tm_mday != log_record_tm.tm_mday))
	{
		printf("We have a new day and path(%s) is over.\n",cFilePathCommand);
		isPrintMkdirOutput = PRINT_MKDIR_OUTPUT_ON;
		// memset(cFilePathCommand, 0x00, 128);
		// sprintf(cFilePathCommand, 
		// 	"cp /root/app/instrument_info/*_%d_%02d_%02d.txt /root/sdcard/app/instrument_info/%d_%02d_%02d/ 2>&1",
		// 	log_record_tm.tm_year + 1900, log_record_tm.tm_mon + 1, log_record_tm.tm_mday,
		// 	log_record_tm.tm_year + 1900, log_record_tm.tm_mon + 1, log_record_tm.tm_mday);
		// memset(cMkdirOutput, 0x00, 256);
		// get_cmd_printf(cFilePathCommand, cMkdirOutput, 256);
		// if(strlen(cMkdirOutput) > 0)
		// {
		//     printf("Command <%s> error: errorInfo is %s\n", cFilePathCommand, cMkdirOutput);
		// }
		memcpy(&log_record_tm, localtime(&timeNow), sizeof(struct tm));
	}
}

/***********************************************************************
 * 函数名：append_logcontent_to_file
 *   功能：把记录内容写入文件。
 * 入口参数： 文件名和文件内容。
 *   返回值： 写入成功返回0，写入失败返回-1。
 ***********************************************************************/
int append_logcontent_to_file(char * cFileName, char * cFileContent)
{
    char cMkdirOutput[256] = {0};
    // char cRemountOutput[256];
    
    char cFilePathCommand[128] = {0};
    char cFilePathWithName[128] = {0};
    int   append_fd; // , send_res;

	// return append_file(cFileName, cFileContent);
	// Write log , We must do it firstly otherwise the following operation would make timestamp error.
    sprintf(cFilePathWithName, "/root/app/instrument_info/%s", cFileName);
    append_fd = open(cFilePathWithName, O_RDWR | O_APPEND);
    if (append_fd == -1) {
		memset(cFilePathCommand, 0x00, 128);
	    sprintf(cFilePathCommand, "mkdir -p /root/app/instrument_info/ 2>&1");
		memset(cMkdirOutput, 0x00, 256);
		get_cmd_printf(cFilePathCommand, cMkdirOutput, 256);
		if(strlen(cMkdirOutput) > 0)
		{
		    printf("Command <%s> error: errorInfo is %s\n", cFilePathCommand, cMkdirOutput);
		}
        append_fd = open(cFilePathWithName, O_RDWR | O_CREAT);
        if (append_fd == -1) {
	        printf("append_file: O_CREAT failed return %d\n", append_fd);
            return -1;
        }
    }
	write(append_fd, cFileContent, strlen(cFileContent));
	// printf("append_file: write %s in the %s.\n", cFileContent, cFileName);
	close(append_fd);
	// End of Write log 
	
	/********************************************************************
 	 * 当天的日志写在内部存储上，每当日期变化，把之前的日志移动到SD卡上。
	 ********************************************************************/
	// printf("------------start of sdcard_operation_process------------\n");
	sdcard_operation_process();
	// printf("------------ end of sdcard_operation_process ------------\n");
    return 0;
}

void out_instrument_history_timer_handler(int signum) {
    // 执行定时器到期时需要做的操作
    // 注意：定时器处理函数应该尽量保持简短，避免执行耗时操作
	out_instrument_reading_chart_history_record(time(NULL));
}

/* 启动一个定时器，用于记录仪表读数。 */
void set_out_instrument_history_timer(int seconds) {
    struct itimerval timer;
    timer.it_value.tv_sec = seconds;  // 第一次定时器到期的秒数
    timer.it_value.tv_usec = 0;       // 第一次定时器到期的微秒数
    // timer.it_interval = timer.it_value;
    timer.it_interval.tv_sec = seconds;  // 定时器周期的秒数（如果为0，则只执行一次）
    timer.it_interval.tv_usec = 0;       // 定时器周期的微秒数

    // 设置定时器信号处理函数
    // signal(SIGALRM, timer_handler);
    struct sigaction act;
    act.sa_handler = out_instrument_history_timer_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask); 
    sigaction(SIGALRM,&act,NULL); //设置信号 SIGALRM 的处理函数为 timer_handler

    // 启动定时器
    setitimer(ITIMER_REAL, &timer, NULL);
}

/*停止setitimer定时器*/
void delete_out_instrument_history_setitimer() 
{
    struct itimerval value; 
    value.it_value.tv_sec = 0; 
    value.it_value.tv_usec = 0; 
    value.it_interval = value.it_value; 
    setitimer(ITIMER_REAL, &value, NULL); 
}

/***********************************************************************
 * 函数名：thread_instrument_Protocol
 *   功能：仪表协议通信线程。
 * 入口参数： 无。
 *   返回值： 无须返回。
 ***********************************************************************/
static void* thread_instrument_Protocol(void *arg)
{
    int   convert_protocol_fd = 0; // , send_res;
	convert_protocol_fd = open_ptc_port();
    // printf("uart Open...\n");
 
	pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
	memset(IReg, 0x00, sizeof(int16_t) * IREG_MAX);
    pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
    // 2.1 设置串口参数
	Protocol_Init(convert_protocol_fd);
    // 2.2 启动一个定时器。每隔INSTRUMENT_HISTORY_TIME_SPAN秒记录一次仪表读数。
	set_out_instrument_history_timer(INSTRUMENT_HISTORY_TIME_SPAN);
    // 2.3 处理仪表通信。
	while (1)
	{
		Protocol_Proc(convert_protocol_fd);
	}
	delete_out_instrument_history_setitimer();
    close(convert_protocol_fd);
    return (void*)NULL;
}

#define   SYS_CONFIG_FILE_NAME    "/root/app/sys_config.ini"
void SysConfig_Init() {
    if(access(SYS_CONFIG_FILE_NAME, F_OK) != 0) 
	{
		create_keyvalue_in_inifile("System", "ServiceID", "15", SYS_CONFIG_FILE_NAME);
		append_keyvalue_in_inifile("ModusUartRspInterval", "20", SYS_CONFIG_FILE_NAME);
	}
}

/***********************************************************************
 * 函数名：thread_modbus_operation
 *   功能：Modbus协议通信线程。
 * 入口参数： 无。
 *   返回值： 无须返回。
 ***********************************************************************/
static void* thread_modbus_operation(void *arg)
{
	int iServerID = SERVER_ID;
	int iSpanCount = 0 ;	
	int modbus_fd = 0;
	struct termios old_tios;
		
    g_iModbusUsartOfflineStatus = MODBUS_USART_ONLINE;
    time_t timeNow = time(NULL);
	uint8_t query[MODBUS_MAX_ADU_LENGTH];
	
    char value[20] = { 0 };
	int iRet = get_ini_key_string("System", "ServiceID", value, SYS_CONFIG_FILE_NAME);
	printf("get_ini_key_string get %s and return %d\n", value, iRet);
    if(iRet == 0)
    {
		iServerID = atoi(value);
    }
	// iRet = get_ini_key_string("System", "ModusUartRspInterval", value, SYS_CONFIG_FILE_NAME);
	// printf("get_ini_key_string get %s and return %d\n", value, iRet);
    // if(iRet == 0)
    // {
	//  	modbus_uart_response_interval = atoi(value) * 1000;
    // }
	 
	modbus_fd = open_and_new_rtu_slave(&old_tios);
	printf("open_and_new_rtu_slave return %d\n", modbus_fd);
	Modbus_Init(iServerID, query);
	// printf("Modbus_Init set iServerID = %d and modbus_uart_response_interval = %d\n", 
	// 	iServerID, modbus_uart_response_interval);
	printf("Modbus_Init set iServerID = %d\n", iServerID);
	// TimerInit();
	V3S_GPIO_SetPin(V3S_PB, 2, 0);
	usleep(5);
	
	while (1)
	{
		//获取查询请求报文
		int ret = 0;
		{
			int s_rc;
			fd_set rset;
			struct timeval tv;
			FD_ZERO(&rset);
			FD_SET(modbus_fd, &rset);
			tv.tv_sec = 0;
			// tv.tv_usec = 50000;
			tv.tv_usec = 20;
			s_rc = select(modbus_fd+1, &rset, NULL, NULL, &tv);
			iSpanCount = iSpanCount + 1;
			if (s_rc > 0) {
				ret = read(modbus_fd, query, MODBUS_MAX_ADU_LENGTH);
			}
		}
		
		if (ret >= 8)
		{
			// printf("read ends with ctx_modbus_uart = %d and return %d\n", 
			//					modbus_fd, ret);
			ret = Modbus_FrameAnalysis(ret);
			// printf("Start of Modbus_FrameAnalysis return %d\n", ret);
			// for(int i = 0 ; i < ret; i++)
			// {
			// 		printf("<%02X> ", query[i]);
			// }
			// printf("\nEnd of Modbus_FrameAnalysis return %d\n", ret);
			
			// 下面的三个延时时间3000, 500, 100都是根据逻辑分析仪的分析结果得到的。
			// 1. 其中收到数据以后，可以多等一会。也就是3ms。
			// 2. DE翻转以后，需要稍微等待一下，以便于获取数据总线，也就是0.5ms。
			// 3. 发送完成以后，需要尽快翻转回去。
			//    用于接收PC端在收到响应以后再次发出的数据。也就是0.1ms。
			// 当PC端发送时间间隔为20ms的时候，数据收发没有错误。
			// 当然如果PC端发送时间间隔过短，例如小于10ms还是会出现非常低概率的错误。
			usleep(3000);
			V3S_GPIO_SetPin(V3S_PB, 2, 1);
		    // 做一点延时，避免发的太快，导致电脑时序混乱。
			usleep(500);
			write(modbus_fd, query, ret);
			// 等待数据发送完成
			tcdrain(modbus_fd);
			usleep(100);
	        V3S_GPIO_SetPin(V3S_PB, 2, 0);
			
			// printf("write socket_id %d and select for %d\n", ret, iSpanCount);
			iSpanCount = 0;
		}
		else if (ret > 0) {
			printf("Wrong Length: mb_data_buf[0] check failed and mb_data_buf[0] is %d\n", query[0]);
			printf("Start of recv and frm_len is %d\n", ret);
			for(int i = 0 ; i < ret; i++)
			{
				printf("<%02X> ", query[i]);
			}
			printf("\nEnd of recv and frm_len is %d\n", ret);
		}
		// We do not receive any data
		// else if (ret == 0) 
		
		// Check MODBUS_USART_ONLINE or MODBUS_USART_OFFLINE
		if (ret > 0) {
			timeNow = time(NULL);
			if(g_iModbusUsartOfflineStatus == MODBUS_USART_OFFLINE)
			{
				g_iModbusUsartOfflineStatus = MODBUS_USART_ONLINE;
				printf("[%s:%s:%d] start out_usart_info_record ONLINE because modbus_receive returns %d\n",
						__FILE__, __FUNCTION__, __LINE__, ret);
				out_modbus_usart_info_record(g_iModbusUsartOfflineStatus, timeNow);
			}
		}
		else if (ret == 0)
		{
			if(g_iModbusUsartOfflineStatus == MODBUS_USART_ONLINE)
			{
				// 只有在连续5秒钟接收不到数据情况下，才会认为Modbus总线无数据。
				if(time(NULL) - timeNow > 10)
				{
					printf("[%s:%s:%d] time(NULL) - timeNow = %d\n",
							__FILE__, __FUNCTION__, __LINE__, (int)(time(NULL) - timeNow));
					g_iModbusUsartOfflineStatus = MODBUS_USART_OFFLINE;
					timeNow = time(NULL);
					printf("[%s:%s:%d] start out_usart_info_record OFFLINE because modbus_receive returns %d\n",
							__FILE__, __FUNCTION__, __LINE__, ret);
					out_modbus_usart_info_record(g_iModbusUsartOfflineStatus, timeNow);
				}
			}
		}
		else if (ret < 0)
		{
			if(g_iModbusUsartOfflineStatus == MODBUS_USART_ONLINE)
			{
				g_iModbusUsartOfflineStatus = MODBUS_USART_OFFLINE;
				timeNow = time(NULL);
				printf("[%s:%s:%d] start out_usart_info_record OFFLINE because modbus_receive returns %d\n",
						__FILE__, __FUNCTION__, __LINE__, ret);
				out_modbus_usart_info_record(g_iModbusUsartOfflineStatus, timeNow);
			}
		}
		check_modbus_usart_info_record(g_iModbusUsartOfflineStatus);
		// Unit Reset
	    // printf("Unit Reset with iSpanCount = %d\n", iSpanCount);
	    
		uint16_t hr_unit_reset;
		pthread_rwlock_rdlock(&hreg_rwlock); // 获取HReg的读锁
		hr_unit_reset = HReg[HR_UNIT_RESET];
		pthread_rwlock_unlock(&hreg_rwlock); // 释放HReg的读锁
		if(hr_unit_reset)
		{
	        printf("Unit Reset with HReg[HR_UNIT_RESET] = %d\n", hr_unit_reset);
			// while(1)
			// {
			// }
			system("reboot");
			break;
		}

	}

	close_and_free_rtu_slave(modbus_fd, &old_tios); // , map);

    printf("uart Close...\n");
    return (void*)NULL;
}


/***********************************************************************
 * 函数名：thread_v3s_udp_controller
 *   功能：读取Modbus配置信息，修改Modbus配置信息
 *         获取最新仪表读数。
 * 入口参数： 无。
 *   返回值： 无须返回。
 ***********************************************************************/
#define V3S_UDP_CONTROLLER_BUFFER_SIZE 1024
static void* thread_v3s_udp_controller(void *arg)
{
    int   sockfd = 0; // , send_res;
    char cReceiveBuffer[V3S_UDP_CONTROLLER_BUFFER_SIZE];
    char cSendBuffer[V3S_UDP_CONTROLLER_BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    ssize_t nReceiveDataLen = 0;

	char cDataType = 0;
	int iDataLen  = 0;
	sockfd = v3s_udp_control_server_init();
	while (1)
	{
		iDataLen = 0;
        addr_len = sizeof(client_addr);
        // 接收数据报
		memset(cReceiveBuffer, 0x00, V3S_UDP_CONTROLLER_BUFFER_SIZE);
        nReceiveDataLen = recvfrom(sockfd, cReceiveBuffer, V3S_UDP_CONTROLLER_BUFFER_SIZE, 0,
        						(struct sockaddr *)&client_addr, &addr_len);
	    // printf("nReceiveDataLen is %d.\n", nReceiveDataLen);
        if (nReceiveDataLen < 0) {
            printf("Receive data failed.\n");
            continue;
        }
		if(nReceiveDataLen >= 3)
		{
			memset(cSendBuffer, 0x00, V3S_UDP_CONTROLLER_BUFFER_SIZE);
			iDataLen = cReceiveBuffer[1] * 255 + cReceiveBuffer[2];
			if(iDataLen + 3 != nReceiveDataLen){
	            printf("[%s:%s:%d] Data length is %d and Data length error.\n",
						__FILE__, __FUNCTION__, __LINE__, iDataLen);
				cSendBuffer[0] = MODBUS_CONFIG_SET_RESPONSE;
				cSendBuffer[1] = 0x00;
				cSendBuffer[2] = 0x02;
				cSendBuffer[3] = MODBUS_CONFIG_RESPONSE_BADREQUEST / 256;
				cSendBuffer[4] = MODBUS_CONFIG_RESPONSE_BADREQUEST % 256;  // 400 Bad Request
	        	sendto(sockfd, cSendBuffer, 5, 
	        				0, (struct sockaddr *)&client_addr, addr_len);
            	printf("[%s:%s:%d] Length error: ReceiveData length is %d and data length is %d.\n", 
						__FILE__, __FUNCTION__, __LINE__, 
						nReceiveDataLen, iDataLen);
	            continue;
	        }
 			cDataType = cReceiveBuffer[0];
	        if(cDataType == MODBUS_CONFIG_GET_REQUEST && iDataLen == 0x00)
	        {
				cSendBuffer[0] = MODBUS_CONFIG_GET_RESPONSE;
				cSendBuffer[1] = 0x00;
				cSendBuffer[2] = 0x06;
				pthread_rwlock_rdlock(&hreg_rwlock); // 获取HReg的读锁
				for(int i = 0; i < MODBUS_CONFIG_CP_REGISTER_ADDR_MAX; i++)
				{
					cSendBuffer[i + 3] = HReg[i];
				}
				pthread_rwlock_unlock(&hreg_rwlock); // 释放HReg的读锁
	        	sendto(sockfd, cSendBuffer, MODBUS_CONFIG_CP_REGISTER_ADDR_MAX + 3, 
	        				0, (struct sockaddr *)&client_addr, addr_len);
            	// printf("[%s:%s:%d] GET_REQUEST OK: ReceiveData length is %d and data length is %d.\n", 
				//		__FILE__, __FUNCTION__, __LINE__, 
				//		nReceiveDataLen, iDataLen);
	        }
			else if(cDataType == MODBUS_CONFIG_SET_REQUEST &&
				iDataLen > 0x00 && iDataLen < MODBUS_CONFIG_CP_REGISTER_ADDR_MAX)
	        {
				for(int i = 0; i < iDataLen; i++)
				{
	    			pthread_rwlock_wrlock(&hreg_rwlock); // 获取HReg的写锁
					HReg[i] = cReceiveBuffer[i + 3];
	    			pthread_rwlock_unlock(&hreg_rwlock); // 释放HReg的写锁
					PARAM_Save(i, cReceiveBuffer[i + 3]); 
				}
				// Update Protocol Config
				// Update_Protocol_Config();
				cSendBuffer[0] = MODBUS_CONFIG_SET_RESPONSE;
				cSendBuffer[1] = 0x00;
				cSendBuffer[2] = 0x02;
				cSendBuffer[3] = 0x00;
				cSendBuffer[4] = MODBUS_CONFIG_RESPONSE_OK;  // 200 OK
	        	sendto(sockfd, cSendBuffer, 5, 
	        				0, (struct sockaddr *)&client_addr, addr_len);
            	// printf("[%s:%s:%d] SET_REQUEST OK: ReceiveData length is %d and data length is %d.\n", 
				//		__FILE__, __FUNCTION__, __LINE__, 
				//		nReceiveDataLen, iDataLen);
	        }
			else if(cDataType == INST_GET_LATEST_READINGS_REQUEST && iDataLen == 0x00)
	        {
				char cFloatBuffer[16] = {0};
				char cProtocolDataOutput[1024];
				Protocol_DataOutput(cProtocolDataOutput);
				uint16_t iDataColumnsNumber = Protocol_GetDataColumnsNumber();
				
	            // printf("[%s:%s:%d] iDataColumnsNumber is %d.\n", 
				//       __FILE__, __FUNCTION__, __LINE__, iDataColumnsNumber);
				cSendBuffer[0] = INST_GET_LATEST_READINGS_RESPONSE;
				cSendBuffer[1] = iDataColumnsNumber / 256;
				cSendBuffer[2] = iDataColumnsNumber % 256;
				// Copy all readings
				char * pDataBufPtr = cProtocolDataOutput;
				float fReading = 0.0;
				float* fReadingPtr = &fReading;
				for(int i = 0; i < iDataColumnsNumber; i++)
				{   
				    char * cSeqPtr = strchr(pDataBufPtr, ',');
				    if(cSeqPtr)
				    {   
				        memset(cFloatBuffer, 0x00, 16);
				        memcpy(cFloatBuffer, pDataBufPtr, cSeqPtr - pDataBufPtr);
				        fReading = atof(cFloatBuffer);
				        // printf("cFloatBuffer = %s and fReading = %.6f\n", cFloatBuffer, fReading);
						memcpy(&cSendBuffer[3 + 4 * i], fReadingPtr, 4);
				        pDataBufPtr = cSeqPtr + 1;
				    }   
				    else {
				        printf("[%s:%s:%d] Data length is %d and Data length error.\n", 
						__FILE__, __FUNCTION__, __LINE__, iDataLen);
						cSendBuffer[0] = MODBUS_CONFIG_SET_RESPONSE;
						cSendBuffer[1] = 0x00;
						cSendBuffer[2] = 0x02;
						cSendBuffer[3] = MODBUS_CONFIG_RESPONSE_INTERNAL_ERROR / 256;
						cSendBuffer[4] = MODBUS_CONFIG_RESPONSE_INTERNAL_ERROR % 256;  // 500 Internal Server Error
			        	sendto(sockfd, cSendBuffer, 5, 
			        				0, (struct sockaddr *)&client_addr, addr_len);
			            printf("[%s:%s:%d] Error DataType: DataType is %d.\n", 
								__FILE__, __FUNCTION__, __LINE__, cDataType);
						continue;
				    }   
				}
	        	sendto(sockfd, cSendBuffer, iDataColumnsNumber * 4 + 3, 
	        				0, (struct sockaddr *)&client_addr, addr_len);
	        }
			else 
			{
	            printf("[%s:%s:%d] Data length is %d and Data length error.\n", 
						__FILE__, __FUNCTION__, __LINE__, iDataLen);
				cSendBuffer[0] = MODBUS_CONFIG_SET_RESPONSE;
				cSendBuffer[1] = 0x00;
				cSendBuffer[2] = 0x02;
				cSendBuffer[3] = MODBUS_CONFIG_RESPONSE_BADREQUEST / 256;
				cSendBuffer[4] = MODBUS_CONFIG_RESPONSE_BADREQUEST % 256;  // 400 Bad Request
	        	sendto(sockfd, cSendBuffer, 5, 
	        				0, (struct sockaddr *)&client_addr, addr_len);
	            printf("[%s:%s:%d] Error DataType: DataType is %d.\n", 
						__FILE__, __FUNCTION__, __LINE__, cDataType);
			}
		}
		else 
		{
            printf("Data length is %d and Data length error.\n", iDataLen);
			cSendBuffer[0] = MODBUS_CONFIG_SET_RESPONSE;
			cSendBuffer[1] = 0x00;
			cSendBuffer[2] = 0x02;
			cSendBuffer[3] = MODBUS_CONFIG_RESPONSE_BADREQUEST / 256;
			cSendBuffer[4] = MODBUS_CONFIG_RESPONSE_BADREQUEST % 256;  // 400 Bad Request
        	sendto(sockfd, cSendBuffer, 5, 
        				0, (struct sockaddr *)&client_addr, addr_len);
        	printf("[%s:%s:%d] nReceiveDataLen error: ReceiveData length is %d and data length is %d.\n", 
				__FILE__, __FUNCTION__, __LINE__, 
				nReceiveDataLen, iDataLen);
		}
	}
    close(sockfd);
    return (void*)NULL;
}

//RTU模式的Slave端程序
int main(int argc, char ** argv)
{
    pthread_t instrument_thread;
    pthread_t instrument_control_thread;
    pthread_t modbus_operation_thread;
	// int ret = 0;
	V3S_GPIO_Init();
    V3S_GPIO_ConfigPin(V3S_PB, 2, V3S_OUT);
    V3S_GPIO_ConfigPin(V3S_PE, 0, V3S_IN);
	
    g_iBatteryOfflineStatus = BATTERY_ONLINE;
    time_t timeNow = time(NULL);
	// log_record_tm =  = localtime(&timeNow);
    memcpy(&log_record_tm, localtime(&timeNow), sizeof(struct tm));
    memcpy(&last_log_record_tm, localtime(&timeNow), sizeof(struct tm));

	memset(modbus_uart_device, 0x00, 20);
	memset(instrument_uart_device, 0x00, 20);
	if(argc == 3) {
		if(strcmp(argv[1], argv[2]) != 0)
		{
			if(strlen(argv[1]) <= 20)
			{
				strncpy(modbus_uart_device, argv[1], strlen(argv[1]));
			}
			if(strlen(argv[2]) <= 20)
			{
				strncpy(instrument_uart_device, argv[2], strlen(argv[2]));
			}
		}
	}

	// Init_All_Periph
	PARAM_Init();
	SysConfig_Init();
	// Init HReg without any lock
	memcpy(&HReg[CP_EEP_BASE], &CP_DefaultValue[0], sizeof(uint16_t) * CP_EEP_MAX);
	PARAM_Reload(0, (uint16_t*)&HReg[CP_EEP_BASE], CP_EEP_MAX);
	HReg[HR_SW_VERSION]= FIRMWARE_VERSION;

	// 初始化HReg的读写锁
    if (pthread_rwlock_init(&hreg_rwlock, NULL) != 0) {
        printf("Failed to initialize rwlock\n");
        return 1;
    }
	// 初始化IReg的读写锁
    if (pthread_rwlock_init(&ireg_rwlock, NULL) != 0) {
        printf("Failed to initialize rwlock\n");
        return 1;
    }

	pthread_create(&modbus_operation_thread, NULL, thread_modbus_operation, NULL);
	// printf("Start Protocol_Proc and ret return %d\n", ret);
	pthread_create(&instrument_thread, NULL, thread_instrument_Protocol, NULL);
	// 读取Modbus配置信息，修改Modbus配置信息。         获取最新仪表读数。
	pthread_create(&instrument_control_thread, NULL, thread_v3s_udp_controller, NULL);
	
	//5. 循环接受客户端请求，并且响应客户端
	while (1)
	{
		//////////////////////////////////////////////////////////////////////////
		// Check battery status
		if((get_battery_status() == BATTERY_OFFLINE)
			&& (g_iBatteryOfflineStatus == BATTERY_ONLINE))
		{
			g_iBatteryOfflineStatus = BATTERY_OFFLINE;
			timeNow = time(NULL);
			out_battery_info_record(g_iBatteryOfflineStatus, timeNow);
		}
		else if((get_battery_status() == BATTERY_ONLINE)
			&& (g_iBatteryOfflineStatus == BATTERY_OFFLINE))
		{
			g_iBatteryOfflineStatus = BATTERY_ONLINE;
			timeNow = time(NULL);
			out_battery_info_record(g_iBatteryOfflineStatus, timeNow);
		}
		check_battery_info_record(g_iBatteryOfflineStatus);
		sleep(10);
	}

    // 销毁HReg的读写锁
    pthread_rwlock_destroy(&hreg_rwlock);
    // 销毁IReg的读写锁
    pthread_rwlock_destroy(&ireg_rwlock);

	printf("Quit the loop: %s\n", strerror(errno));
	return 0;
}

