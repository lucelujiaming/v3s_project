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

#include "param.h"

#include "modbus.h"
#include "usart.h"

#include "Protocol/protocol.h"
#include "Protocol/Modbus/ModbusSlave.h"

#include "modbus_rtu.h"
#include "config_operation.h"

#include "v3s_gpio_operation.h"

#define MODBUS_MAX_ADU_LENGTH  260


//从站地址 17
#define SERVER_ID 17

#define MODBUS_UART_DEVICE       "/dev/ttyS0"
#define INSTRUMENT_UART_DEVICE   "/dev/ttyS1"

// output_mix_history_trend
#define    TIME_SPAN               900 //   15mins   600   // 10 minutes
// #define    NEW_YEAR_DAY_2024    1704038400

#define  INSTRUMENT_ONLINE             0
#define  BATTERY_OFFLINE               1
#define  USART_OFFLINE                 2
#define  INSTRUMENT_STATUS             3

char modbus_uart_device[20];
char instrument_uart_device[20];

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
int append_file(char * cFileName, char * cFileContent);


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

int close_and_free_rtu_slave(int modbus_fd, struct termios* old_tios)
{
    if (modbus_fd != -1) {
        tcsetattr(modbus_fd, TCSANOW, old_tios);
        close(modbus_fd);
        modbus_fd = -1;
    }
	return 0;
}


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
    if (instrument_fd < 0) {
        perror("INSTRUMENT_UART_DEVICE open error");
        exit(1);
    }
	return instrument_fd;
}

void out_instrument_history_record(
        int iParticleFirst,   int iParticleSecond, 
        int iParticleThird,   int iParticleFourth, 
        int iParticleFifth,   int iParticleSixth,  
        int iParticleSeventh, int iParticleEighth, 
        int iLaserRefFirst,  int iLaserRefSecond, time_t iFakeTimeStamp)
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
    struct tm*     tmNow    = localtime(&timeNow);

    sprintf(cFileContent, "\"%04d-%02d-%02d %02d:%02d:%02d\",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, 
            tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec,
            iParticleFirst, iParticleSecond, iParticleThird, iParticleFourth, 
            iParticleFifth, iParticleSixth, iParticleSeventh, iParticleEighth, 
            iLaserRefFirst, iLaserRefSecond);

    sprintf(cFileName, "instrument_history_record_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    append_file(cFileName, cFileContent);
	
    struct tm*     tmToday  = localtime(&timeNow);
    tmToday->tm_hour = tmToday->tm_min = tmToday->tm_sec = 0;
    // time_t timeToday = mktime(tmToday);
    
    // sprintf(cFileContent, "[%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d],\r\n",
    sprintf(cFileContent, "[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d],\r\n",
            // timeNow - timeToday,
            iParticleFirst, iParticleSecond, iParticleThird, iParticleFourth, 
            iParticleFifth, iParticleSixth, iParticleSeventh, iParticleEighth, 
            iLaserRefFirst, iLaserRefSecond);

    sprintf(cFileName, "instrument_history_info_record_unixtime_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    printf("We output the instrument_history_info_record_unixtime_%04d_%02d_%02d.txt at %ld.\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, timeNow);
    append_file(cFileName, cFileContent);

}

void out_battery_info_record(int iOfflineStatus, time_t iFakeTimeStamp)
{
    static int iLastOfflineStatus = INSTRUMENT_STATUS;
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
    append_file(cFileName, cFileContent);
    
    if(iLastOfflineStatus != iOfflineStatus)
    {
        iLastOfflineStatus = iOfflineStatus;
        
        sprintf(cFileName, "battery_info_switch_record_%04d_%02d_%02d.txt", 
                tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
        append_file(cFileName, cFileContent);
    }
	
    struct tm*     tmToday  = localtime(&timeNow);
    tmToday->tm_hour = tmToday->tm_min = tmToday->tm_sec = 0;
    // time_t timeToday = mktime(tmToday);
	
    // sprintf(cFileContent, "[%ld,%d],\r\n",
    //        timeNow - timeToday, iOfflineStatus);
    sprintf(cFileContent, "%d,\r\n", iOfflineStatus);

    sprintf(cFileName, "battery_info_record_unixtime_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    printf("We output the battery_info_record_unixtime_%04d_%02d_%02d.txt at %ld.\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, timeNow);
    append_file(cFileName, cFileContent);
}

void out_usart_info_record(int iOfflineStatus, time_t iFakeTimeStamp)
{
    static int iLastOfflineStatus = INSTRUMENT_STATUS;
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

    sprintf(cFileName, "usart_info_record_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    append_file(cFileName, cFileContent);
	
    if(iLastOfflineStatus != iOfflineStatus)
    {
        iLastOfflineStatus = iOfflineStatus;
        
        sprintf(cFileName, "usart_info_switch_record_%04d_%02d_%02d.txt", 
                tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
        append_file(cFileName, cFileContent);
    }
    
    struct tm*     tmToday  = localtime(&timeNow);
    tmToday->tm_hour = tmToday->tm_min = tmToday->tm_sec = 0;
    // time_t timeToday = mktime(tmToday);
    
    // sprintf(cFileContent, "[%ld,%d],\r\n",
    //        timeNow - timeToday, iOfflineStatus);
    sprintf(cFileContent, "%d,\r\n", iOfflineStatus);

    sprintf(cFileName, "usart_info_record_unixtime_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    // printf("We output the usart_info_record_unixtime_%04d_%02d_%02d.txt at %ld.\r\n",
    //         tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, timeNow);
    append_file(cFileName, cFileContent);
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

int get_battery_status()
{
    unsigned int uRet = V3S_GPIO_GetPin(V3S_PE, 0);
    
    if(uRet == 0)
    {
        return BATTERY_OFFLINE;
    }
    else 
    {
        return INSTRUMENT_ONLINE;
    }
}

int append_file(char * cFileName, char * cFileContent)
{
	// int iRetryCount = 0;
	// int iRet = 0;
    char cMkdirOutput[256];
    // char cRemountOutput[256];
	
    char cFilePathWithName[128];
    char cFilePathMkdirCommand[128];
    int   append_fd; // , send_res;
    time_t timeNow = time(NULL);
    struct tm*     tmNow    = localtime(&timeNow);
	
    sprintf(cFilePathWithName, "/root/sdcard/app/instrument_info/%d_%02d_%02d/%s",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, cFileName);
    // 2. 打开PTC私有协议对应的串口
    append_fd = open(cFilePathWithName, O_RDWR | O_APPEND);
    if (append_fd < 0) {
		// iRetryCount = 0;
		while(1)
		{
			// 这里的2>&1表示将标准错误（文件描述符2）重定向到标准输出（文件描述符1）。
		    sprintf(cFilePathMkdirCommand, "mkdir -p /root/sdcard/app/instrument_info/%d_%02d_%02d/ 2>&1",
		            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
			memset(cMkdirOutput, 0x00, 256);
 	        // iRet = 
			get_cmd_printf(cFilePathMkdirCommand, cMkdirOutput, 256);
		    // printf("get_cmd_printf(%s) return %d\n",cFilePathMkdirCommand, iRet);
			if(strlen(cMkdirOutput) > 0)
			{
		        // printf("Command <%s> error: errorInfo is %s\n", cFilePathMkdirCommand, cMkdirOutput);
				// memset(cRemountOutput, 0x00, 256);
    			// get_cmd_printf("/root/app/remount_sdcard.sh", cRemountOutput, 256);
		        // printf("remount_sdcard return %s\n", cRemountOutput);
		        return -1;
			}
			else 
			{
		        // printf("mkdir error: errorInfo is empty with <%s>\n", cMkdirOutput);
				break;
			}
		}
		
        append_fd = open(cFilePathWithName, O_RDWR | O_CREAT);
        if (append_fd < 0) {
	        // printf("append_file: open failed return %d\n", append_fd);
            return -1;
        }
    }
	// printf("append_file: write %s return %d\n", cFileContent, append_fd);
	write(append_fd, cFileContent, strlen(cFileContent));
	close(append_fd);
    return 0;
}

static void* thread_instrument_Protocol(void *arg)
{
    int   convert_protocol_fd = 0; // , send_res;
	convert_protocol_fd = open_ptc_port();
    // printf("uart Open...\n");
 
    // 2.1 设置串口参数
	Protocol_Init(convert_protocol_fd);
	while (1)
	{
		Protocol_Proc(convert_protocol_fd);
	}
    close(convert_protocol_fd);
    return (void*)NULL;
}

#define   SYS_CONFIG_FILE_NAME    "sys_config.ini"


void SysConfig_Init() {
    if(access(SYS_CONFIG_FILE_NAME, F_OK) != 0) 
	{
		create_keyvalue_in_inifile("System", "ServiceID", "15", SYS_CONFIG_FILE_NAME);
	}
}

static void* thread_modbus_operation(void *arg)
{
	int iServerID = SERVER_ID;
	int iSpanCount = 0 ;	
	int modbus_fd = 0;
	struct termios old_tios;
		
    time_t timeNow = time(NULL);
	uint8_t query[MODBUS_MAX_ADU_LENGTH];
	
    char value[20] = { 0 };
	int iRet = get_ini_key_string("System", "ServiceID", value, SYS_CONFIG_FILE_NAME);
	printf("get_ini_key_string get %s and return %d\n", value, iRet);
    if(iRet == 0)
    {
		iServerID = atoi(value);
    }
	modbus_fd = open_and_new_rtu_slave(&old_tios);
	printf("open_and_new_rtu_slave return %d\n", modbus_fd);
	Modbus_Init(iServerID, query);
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
		
		if (ret > 0)
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
			V3S_GPIO_SetPin(V3S_PB, 2, 1);
		    // 做一点延时，避免发的太快，导致电脑时序混乱。
			usleep(1000);
			write(modbus_fd, query, ret);
			// 等待数据发送完成
			tcdrain(modbus_fd);
			// 55chars need 5729 us 
	        V3S_GPIO_SetPin(V3S_PB, 2, 0);
			
			// printf("write socket_id %d and select for %d\n", ret, iSpanCount);
			iSpanCount = 0;
		}
		else
		{
			timeNow = time(NULL);
			// printf("start out_usart_info_record USART_OFFLINE because modbus_receive returns %d\n", ret);
			out_usart_info_record(USART_OFFLINE, timeNow);
			// printf("end out_usart_info_record USART_OFFLINE because modbus_receive returns %d\n", ret);
		}
		// Unit Reset
	    // printf("Unit Reset with iSpanCount = %d\n", iSpanCount);
		if(HReg[HR_UNIT_RESET])
		{
	        printf("Unit Reset with HReg[HR_UNIT_RESET] = %d\n", HReg[HR_UNIT_RESET]);
			while(1)
			{
			}
		}

	}

	close_and_free_rtu_slave(modbus_fd, &old_tios); // , map);

    printf("uart Close...\n");
    return (void*)NULL;
}

//RTU模式的Slave端程序
int main(int argc, char ** argv)
{
    pthread_t instrument_thread;
    pthread_t modbus_operation_thread;
	// int ret = 0;
	V3S_GPIO_Init();
    V3S_GPIO_ConfigPin(V3S_PB, 2, V3S_OUT);
    V3S_GPIO_ConfigPin(V3S_PE, 0, V3S_IN);
	
    // int iUsartOfflineStatus   = INSTRUMENT_ONLINE;
    int iBatteryOfflineStatus = INSTRUMENT_ONLINE;
    time_t timeNow = time(NULL);

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

	PARAM_Init();
	SysConfig_Init();
	memcpy(&HReg[CP_EEP_BASE], &CP_DefaultValue[0], sizeof(uint16_t) * CP_EEP_MAX);
	PARAM_Reload(0, (uint16_t*)&HReg[CP_EEP_BASE], CP_EEP_MAX);
	HReg[HR_SW_VERSION]= FIRMWARE_VERSION;

	pthread_create(&modbus_operation_thread, NULL, thread_modbus_operation, NULL);
	// printf("Start Protocol_Proc and ret return %d\n", ret);
	pthread_create(&instrument_thread, NULL, thread_instrument_Protocol, NULL);
	
	//5. 循环接受客户端请求，并且响应客户端
	while (1)
	{
		//////////////////////////////////////////////////////////////////////////
		// Check battery status
		if((get_battery_status() == BATTERY_OFFLINE)
			&& (iBatteryOfflineStatus == INSTRUMENT_ONLINE))
		{
			iBatteryOfflineStatus = BATTERY_OFFLINE;
			out_battery_info_record(iBatteryOfflineStatus, timeNow);
		}
		else if((get_battery_status() == INSTRUMENT_ONLINE)
			&& (iBatteryOfflineStatus == BATTERY_OFFLINE))
		{
			iBatteryOfflineStatus = INSTRUMENT_ONLINE;
			timeNow = time(NULL);
			out_battery_info_record(iBatteryOfflineStatus, timeNow);
		}
		sleep(10);
	}

	printf("Quit the loop: %s\n", strerror(errno));
	return 0;
}

