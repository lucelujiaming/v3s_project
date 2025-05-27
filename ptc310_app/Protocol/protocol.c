//protocol.c
#include <stdio.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include <time.h>
#include "timer.h"
#include "mbvardef.h"
#include "protocol.h"
#include "usart_operate.h"

#include "v3s_gpio_operation.h"

#include "pms.h"
#include "meeco.h"
#include "jag.h"
#include "peak.h"
#include "delta_f.h"
#include "tiger.h"
#include "orthodyne.h"
#include "saes.h"
#include "ametek.h"
#include "teledyne.h"
#include "nanochrome.h"
#include "k1000a.h"
#include "hctm.h"
#include "reliya.h"

typedef struct
{
	void (*init_proc)(int fd, uint16_t addr);
	
	uint16_t (*request_proc)(void);
	uint8_t (*analysis_proc)(uint16_t len);
	// 
	uint16_t (*get_data_columns_number)();
	uint16_t (*data_output)(char* strOutput);
	

}PROTOCOL_DEF;

int16_t HReg[HREG_MAX] = {0};
int16_t IReg[IREG_MAX] = {0};

// 2. 定义全局变量和读写锁
pthread_rwlock_t hreg_rwlock;
pthread_rwlock_t ireg_rwlock;

char data_temp[100];
int16_t* protocol_buff;
uint8_t* serial_buff;
uint8_t little_endian;

extern char instrument_uart_device_protocal[32];

PTC310_INTERFACE_STATUS g_iInstrumentUsartOfflineStatus = INSTRUMENT_USART_ONLINE;

PROTOCOL_DEF ProtocolProcList[PROTOCOL_MAX]=
{
	{PMS_Init_1, NULL, PMS_PDS_PA_Analysis, 
					PMS_PDS_PA_DataColumnsNumber, PMS_PDS_PA_DataOutput},
	{PMS_Init_1, NULL, PMS_HPGP_101_Analysis, 
					PMS_HPGP_101_DataColumnsNumber, PMS_HPGP_101_DataOutput},
	{PMS_Init_2, NULL, PMS_LASAIR_III_Analysis, 
					PMS_LASAIR_III_DataColumnsNumber, PMS_LASAIR_III_DataOutput},
	{MEECO_Init, MEECO_Request, MEECO_Analysis, 
					MEECO_DataColumnsNumber, MEECO_DataOutput},	
	{JAG_Init, JAG_Request, JAG_Analysis, 
					JAG_DataColumnsNumber, JAG_DataOutput},
	
	{PEAK_Init, NULL, PEAK_Analysis, 
					PEAK_DataColumnsNumber, PEAK_DataOutput},
	{DELTAF_Init, DELTAF_Request, DELTAF_Analysis, 
					DELTAF_DataColumnsNumber, DELTAF_DataOutput},
	{TIGER_Init, TIGER_Request, TIGER_Analysis, 
					TIGER_DataColumnsNumber, TIGER_DataOutput},
	{ORTHODYNE_Init, NULL, ORTHODYNE_Analysis, 
					ORTHODYNE_DataColumnsNumber, ORTHODYNE_DataOutput},
	{SAES_Init, NULL, SAES_Analysis, 
					SAES_DataColumnsNumber, SAES_DataOutput},
	
	{AMETEK_Init, AMETEK_5000_Request, AMETEK_5000_Analysis, 
					AMETEK_5000_DataColumnsNumber, AMETEK_5000_DataOutput},
	{AMETEK_Init, AMETEK_2850_Request, AMETEK_2850_Analysis, 
					AMETEK_2850_DataColumnsNumber, AMETEK_2850_DataOutput},
	{TELEDYNE_Init, NULL, TELEDYNE_Analysis, 
					TELEDYNE_DataColumnsNumber, TELEDYNE_DataOutput},
	{SERVOMEX_NANO_Init, SERVOMEX_NANO_Request, SERVOMEX_NANO_Analysis, 
					SERVOMEX_NANO_DataColumnsNumber, SERVOMEX_NANO_DataOutput},
	{SERVOMEX_K1000A_Init, NULL, SERVOMEX_K1000A_Analysis, 
					SERVOMEX_K1000A_DataColumnsNumber, SERVOMEX_K1000A_DataOutput},
	
	{HCTM_Init, NULL, HCTM_WCPC0703E_Analysis, 
					HCTM_WCPC0703E_DataColumnsNumber, HCTM_WCPC0703E_DataOutput},
	{PMS_Init_2, NULL, PMS_PDS_E_Analysis, 
					PMS_PDS_E_DataColumnsNumber, PMS_PDS_E_DataOutput},
	{RELIYA_Init, NULL, RELIYA_HGPC_100_Analysis, 
					RELIYA_HGPC_100_DataColumnsNumber, RELIYA_HGPC_100_DataOutput}
};

PROTOCOL_DEF *ProtocolConvert;

uint8_t comm_err_cnt;

// 初始化为非循环定时器，未使能，超时时间为2秒。
static volatile MTIMER tm_FrmReq= {"FrmReq", 0,false,false,{0, 0},2000};
// 初始化为非循环定时器，未使能，超时时间为1秒。
static volatile MTIMER tm_FrmAckTo= {"FrmAckTo", 0,false,false,{0, 0},1000};

void record_uart_ptc_status(int status);

//  void Update_Protocol_Config()
//  {
//  	uint16_t prot_id;
//  	uint16_t inst_addr;
//  	uint32_t req_time;
//  	uint32_t ack_time;
//  	
//  	prot_id= HReg[CP_PROTOCOL_ID];
//  	inst_addr= HReg[CP_INSTRUMENT_ADDR];
//  	req_time= (uint32_t)HReg[CP_ENQUIRY_TIME]*100;
//  	ack_time= (uint32_t)HReg[CP_RESPONSE_TIME]*100;
//  	little_endian= HReg[CP_32BIT_LE];
//  }

void Protocol_Init(int fd)
{
	uint16_t prot_id;
	uint16_t inst_addr;
	uint32_t req_time;
	uint32_t ack_time;
	
	//Variables Initial
    pthread_rwlock_rdlock(&hreg_rwlock); // 获取HReg的读锁
	prot_id= HReg[CP_PROTOCOL_ID];
	
	inst_addr= HReg[CP_INSTRUMENT_ADDR];
	
	req_time= (uint32_t)HReg[CP_ENQUIRY_TIME]*100;
	ack_time= (uint32_t)HReg[CP_RESPONSE_TIME]*100;
    pthread_rwlock_unlock(&hreg_rwlock); // 释放HReg的读锁
	
	if(prot_id >= PROTOCOL_MAX)
	{
    	printf("prot_id(%d) >= PROTOCOL_MAX...\n", prot_id);
		return;
	}
	//////////////////////////////////////////////////////////
	ProtocolConvert= &ProtocolProcList[prot_id];
	
	if(prot_id== PT_PEAK && inst_addr== 0)
	{
		ProtocolConvert->request_proc= NULL;
	}
	
	//Protocol Init
	if(ProtocolConvert->init_proc)
	{
		(*(ProtocolConvert->init_proc))(fd, inst_addr);
	}
	
	//
	Timer_SetParam((MTIMER*)&tm_FrmAckTo, false, ack_time);
	
	if(ProtocolConvert->request_proc)
	{
		Timer_SetParam((MTIMER*)&tm_FrmReq, true, req_time);
		Timer_Restart((MTIMER*)&tm_FrmReq);
	
		Timer_Init((MTIMER*)&tm_FrmReq);
	}
	else
	{
		Timer_Restart((MTIMER*)&tm_FrmAckTo);
	}
	
	Timer_Init((MTIMER*)&tm_FrmAckTo);

	// RS485 / RS232
	// USART3_SetInterface();
	
	protocol_buff= IReg;
	// serial_buff= USART3_GetBuf();
    serial_buff= Instrument_USART_GetBuf();
    pthread_rwlock_rdlock(&hreg_rwlock); // 获取HReg的读锁
	little_endian= HReg[CP_32BIT_LE];
    pthread_rwlock_unlock(&hreg_rwlock); // 释放HReg的读锁
	
	g_iInstrumentUsartOfflineStatus = INSTRUMENT_USART_ONLINE;
	
	// Init Timer
	Instrument_USART_Init();
}

void Protocol_Proc(int fd)
{
    time_t timeNow = time(NULL);
	uint16_t len;
	uint8_t err;
	uint16_t cp_fault_times, cp_protocol_id; 
	
    // UART2_RE为低电平时从外接收信息，高电平或者开路禁用接收。UART2_RE为PB2。
    // UART2_DE为高电平时向外发送信息，低电平或者开路禁用发送。UART2_DE为PB3。
    // UART2_RE(PB2)为低电平使能接收信息。
	if(strcmp(instrument_uart_device_protocal, "232") == 0)
	{
		// printf("V3S_GPIO_SetPin(V3S_PB, 2, 1) \n");
		// V3S_GPIO_SetPin(V3S_PB, 2, 1);
	}
	else 
	{
		// printf("V3S_GPIO_SetPin(V3S_PB, 2, 0) \n");
		V3S_GPIO_SetPin(V3S_PB, 2, 0);
	}
    // UART2_DE(PB3)为低电平禁用发送信息。
	V3S_GPIO_SetPin(V3S_PB, 3, 0);
	usleep(500);
	// len= USART3_FrameReceived();
	len= Instrument_USART_FrameReceived(fd);
	// printf("Instrument_USART_FrameReceived read %d\n", len);
	if(len > 0)
	{
		// printf("Protocol_Proc analysis_proc %d\n", len);
		// for(int i = 0 ; i < len; i++)
		// {
		//  	printf("<%02X> ", serial_buff[i]);
		// }
		// printf("\nEnd of Protocol_Proc recv and len is %d\n", len);
		
		if(g_iInstrumentUsartOfflineStatus == INSTRUMENT_USART_OFFLINE)
		{
			g_iInstrumentUsartOfflineStatus = INSTRUMENT_USART_ONLINE;
			// record_uart_ptc_status(uart_ptc_status);
			timeNow = time(NULL);
			printf("[%s:%s:%d] start out_usart_info_record ONLINE because Instrument_USART_FrameReceived returns %d\n",
						__FILE__, __FUNCTION__, __LINE__, len);
			out_instrument_usart_info_record(g_iInstrumentUsartOfflineStatus, timeNow);
		}
		err= (*ProtocolConvert->analysis_proc)(len);
		
		if(err== 0)
		{
			comm_err_cnt= 0;
						
			if(ProtocolConvert->request_proc)
			{
				// printf("Timer_Stop tm_FrmAckTo err=%d\n", err);
				Timer_Stop((MTIMER*)&tm_FrmAckTo);
			}
			else
			{
				printf("[%s:%s:%d] Timer_Restart tm_FrmAckTo err=%d\n", 
                            __FILE__, __FUNCTION__, __LINE__, err);
				Timer_Restart((MTIMER*)&tm_FrmAckTo);
			}
		}
		else
		{
			printf("[%s:%s:%d] Timer_Stop tm_FrmAckTo err=%d\n", 
                            __FILE__, __FUNCTION__, __LINE__, err);
		}
	}
	else if(len == 0)
	{
		// printf("\n");
		// printf("Instrument does not work and nothing read\n");
		if(g_iInstrumentUsartOfflineStatus == INSTRUMENT_USART_ONLINE)
		{
			g_iInstrumentUsartOfflineStatus = INSTRUMENT_USART_OFFLINE;
			// record_uart_ptc_status(uart_ptc_status);
			timeNow = time(NULL);
			printf("[%s:%s:%d] start out_usart_info_record OFFLINE because Instrument_USART_FrameReceived returns %d\n",
						__FILE__, __FUNCTION__, __LINE__, len);
			out_instrument_usart_info_record(g_iInstrumentUsartOfflineStatus, timeNow);
		}
	}
	else if(len < 0)
	{
		printf("Protocol_Proc read error %d\n", len);
		if(g_iInstrumentUsartOfflineStatus == INSTRUMENT_USART_ONLINE)
		{
			g_iInstrumentUsartOfflineStatus = INSTRUMENT_USART_OFFLINE;
			// record_uart_ptc_status(uart_ptc_status);
			timeNow = time(NULL);
			printf("[%s:%s:%d] start out_usart_info_record OFFLINE because Instrument_USART_FrameReceived returns %d\n",
						__FILE__, __FUNCTION__, __LINE__, len);
			out_instrument_usart_info_record(g_iInstrumentUsartOfflineStatus, timeNow);
		}
	}
	check_instrument_usart_info_record(g_iInstrumentUsartOfflineStatus);
	
	//Time to Request
	// printf("Timer_Expires tm_FrmReq\n");
	if(Timer_Expires((MTIMER*)&tm_FrmReq))
	{
		len= (*ProtocolConvert->request_proc)();
		// printf("[%s:%s:%d] Protocol_Proc request_proc %d\n",
		// 				__FILE__, __FUNCTION__, __LINE__, len);
		
		if(len)
		{
		    // printf("Instrument_USART_Send request_proc %d\n", len);
			// USART3_Send(len);
            
            // UART2_RE为低电平时从外接收信息，高电平或者开路禁用接收。UART2_RE为PB2。
            // UART2_DE为高电平时向外发送信息，低电平或者开路禁用发送。UART2_DE为PB3。
            // UART2_RE(PB2)为高电平禁用接收信息。
            
			if(strcmp(instrument_uart_device_protocal, "232") == 0)
			{
				// printf("V3S_GPIO_SetPin(V3S_PB, 2, 1) \n");
				// V3S_GPIO_SetPin(V3S_PB, 2, 1);
			}
			else 
			{
				// printf("V3S_GPIO_SetPin(V3S_PB, 2, 1) \n");
            	V3S_GPIO_SetPin(V3S_PB, 2, 1);
			}
            // UART2_DE(PB3)为高电平使能发送信息。
            V3S_GPIO_SetPin(V3S_PB, 3, 1);
		    // 做一点延时，避免发的太快，导致电脑时序混乱。
			usleep(500);
			
			// printf("Protocol_Proc Instrument_USART_Send %d\n", len);
			// for(int i = 0 ; i < len; i++)
			// {
			//  	printf("<%02X> ", serial_buff[i]);
			// }
			// printf("\nEnd of Protocol_Proc Instrument_USART_Send and len is %d\n", len);
		
            Instrument_USART_Send(fd, len);
		}
	}
	
	//Request Send Complete
	// if(USART3_SendComplete())
    if(Instrument_USART_SendComplete(fd))
	{
		Timer_Restart((MTIMER*)&tm_FrmAckTo);
	}
	//Ack Timeout
	// printf("Timer_Expires tm_FrmAckTo\n");
	if(Timer_Expires((MTIMER*)&tm_FrmAckTo))
	{
		printf("Timer_Expires((MTIMER*)&tm_FrmAckTo)\n");
		pthread_rwlock_rdlock(&hreg_rwlock); // 获取HReg的读锁
		cp_fault_times = HReg[CP_FAULT_TIMES];
		pthread_rwlock_unlock(&hreg_rwlock); // 释放HReg的读锁
		if(++comm_err_cnt > cp_fault_times)
		{
			comm_err_cnt= cp_fault_times;
		}
		
		if(ProtocolConvert->request_proc)
		{
		}
		else
		{
			Timer_Restart((MTIMER*)&tm_FrmAckTo);
		}
	}
	
	pthread_rwlock_rdlock(&hreg_rwlock); // 获取HReg的读锁
	cp_fault_times = HReg[CP_FAULT_TIMES];
	pthread_rwlock_unlock(&hreg_rwlock); // 释放HReg的读锁
	if(comm_err_cnt >= cp_fault_times)
	{
		pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
		IReg[0]= 1;
    	pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
	}
	else
	{
		pthread_rwlock_wrlock(&ireg_rwlock); // 获取IReg的写锁
		IReg[0]= 0;
    	pthread_rwlock_unlock(&ireg_rwlock); // 释放IReg的写锁
	}

	pthread_rwlock_rdlock(&hreg_rwlock); // 获取HReg的读锁
	cp_protocol_id = HReg[CP_PROTOCOL_ID];
	pthread_rwlock_unlock(&hreg_rwlock); // 释放HReg的读锁
	
	if(cp_protocol_id >= PROTOCOL_MAX)
	{
    	pthread_rwlock_wrlock(&hreg_rwlock); // 获取HReg的写锁
		HReg[CP_PROTOCOL_ID]= PROTOCOL_MAX;
   		pthread_rwlock_unlock(&hreg_rwlock); // 释放HReg的写锁
	}
}

// 根据《PTC310_V2.7.3用户手册》为每一个仪表编写数据解析函数。用于图标打印。
// 只支持两种类型。分别是16比特的Int类型和32比特的Real类型。
// 也就是说一个Real类型占用两个IReg元素。
uint16_t Protocol_DataOutput(char * strOutput)
{
	if(ProtocolConvert->data_output)
	{
		return (*ProtocolConvert->data_output)(strOutput);
	}
	return 0;
}

uint16_t Protocol_GetDataColumnsNumber()
{
	if(ProtocolConvert->get_data_columns_number)
	{
		return (*ProtocolConvert->get_data_columns_number)();
	}
	return 0;
}


void record_uart_ptc_status(int status)
{
	char cTemp[128];
    int   uart_ptc_status_fd; // , send_res;
    // 2. 打开PTC私有协议对应的串口
    uart_ptc_status_fd = open("./uart_ptc_status_record", O_RDWR|O_NOCTTY/*|O_NDELAY*/);
	printf("record_uart_ptc_status: open return %d\n", uart_ptc_status_fd);
    if (uart_ptc_status_fd == -1) {
	    printf("record_uart_ptc_status: open failed %d\n", uart_ptc_status_fd);
		close(uart_ptc_status_fd);
        return;
    }
	sprintf(cTemp, "%ld\t%d\r\n", time(NULL), status);
	write(uart_ptc_status_fd, cTemp, strlen(cTemp));
	close(uart_ptc_status_fd);
	
}


