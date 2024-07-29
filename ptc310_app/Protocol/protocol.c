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
#include <time.h>
#include "timer.h"
#include "mbvardef.h"
#include "protocol.h"
#include "usart_operate.h"

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

#define UART_PTC_STATUS_OK        1
#define UART_PTC_STATUS_NG        2



typedef struct
{
	void (*init_proc)(int fd, uint16_t addr);
	
	uint16_t (*request_proc)(void);
	uint8_t (*analysis_proc)(uint16_t len);

}PROTOCOL_DEF;

int16_t HReg[HREG_MAX];
int16_t IReg[IREG_MAX];

char data_temp[100];
int16_t* protocol_buff;
uint8_t* serial_buff;
uint8_t little_endian;

uint8_t uart_ptc_status = UART_PTC_STATUS_OK;



PROTOCOL_DEF ProtocolProcList[PROTOCOL_MAX]=
{
	{PMS_Init_1, NULL, PMS_PDS_PA_Analysis},
	{PMS_Init_1, NULL, PMS_HPGP_101_Analysis},
	{PMS_Init_2, NULL, PMS_LASAIR_III_Analysis},
	{MEECO_Init, MEECO_Request, MEECO_Analysis},	
	{JAG_Init, JAG_Request, JAG_Analysis},
	
	{PEAK_Init, NULL, PEAK_Analysis},
	{DELTAF_Init, DELTAF_Request, DELTAF_Analysis},
	{TIGER_Init, TIGER_Request, TIGER_Analysis},
	{ORTHODYNE_Init, NULL, ORTHODYNE_Analysis},
	{SAES_Init, NULL, SAES_Analysis},
	
	{AMETEK_Init, AMETEK_5000_Request, AMETEK_5000_Analysis},
	{AMETEK_Init, AMETEK_2850_Request, AMETEK_2850_Analysis},
	{TELEDYNE_Init, NULL, TELEDYNE_Analysis},
	{SERVOMEX_NANO_Init, SERVOMEX_NANO_Request, SERVOMEX_NANO_Analysis},
	{SERVOMEX_K1000A_Init, NULL, SERVOMEX_K1000A_Analysis},
	
	{HCTM_Init, NULL, HCTM_WCPC0703E_Analysis},
	{PMS_Init_2, NULL, PMS_PDS_E_Analysis},
	{RELIYA_Init, NULL, RELIYA_HGPC_100_Analysis}
};

PROTOCOL_DEF *ProtocolConvert;

uint8_t comm_err_cnt;

static volatile MTIMER tm_FrmReq= {0,false,false,0,2000};
static volatile MTIMER tm_FrmAckTo= {0,false,false,0,1000};

void record_uart_ptc_status(int status);

void Protocol_Init(int fd)
{
	uint16_t prot_id;
	uint16_t inst_addr;
	uint32_t req_time;
	uint32_t ack_time;
	
	//Variables Initial
	prot_id= HReg[CP_PROTOCOL_ID];
	
	inst_addr= HReg[CP_INSTRUMENT_ADDR];
	
	req_time= (uint32_t)HReg[CP_ENQUIRY_TIME]*100;
	ack_time= (uint32_t)HReg[CP_RESPONSE_TIME]*100;
	
	if(prot_id >= PROTOCOL_MAX)
	{
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
    serial_buff= Convert_USART_GetBuf();
	little_endian= HReg[CP_32BIT_LE];
	
	uart_ptc_status = UART_PTC_STATUS_OK;
}

void Protocol_Proc(int fd)
{
	uint16_t len;
	uint8_t err;
	
	// len= USART3_FrameReceived();
	len= Convert_USART_FrameReceived(fd);
	if(len > 0)
	{
		if(uart_ptc_status == UART_PTC_STATUS_NG)
		{
			uart_ptc_status = UART_PTC_STATUS_OK;
			record_uart_ptc_status(uart_ptc_status);
		}
		err= (*ProtocolConvert->analysis_proc)(len);
		
		if(err== 0)
		{
			comm_err_cnt= 0;
						
			if(ProtocolConvert->request_proc)
			{
				Timer_Stop((MTIMER*)&tm_FrmAckTo);
			}
			else
			{
				Timer_Restart((MTIMER*)&tm_FrmAckTo);
			}
		}
	}
	else if(len == 0)
	{
		printf("Nothing read");
	}
	else if(len < 0)
	{
		if(uart_ptc_status == UART_PTC_STATUS_OK)
		{
			uart_ptc_status = UART_PTC_STATUS_NG;
			record_uart_ptc_status(uart_ptc_status);
		}
	}
	
	//Time to Request
	if(Timer_Expires((MTIMER*)&tm_FrmReq))
	{
		len= (*ProtocolConvert->request_proc)();
		
		if(len)
		{
			// USART3_Send(len);
            Convert_USART_Send(fd, len);
		}
	}
	
	//Request Send Complete
	// if(USART3_SendComplete())
    if(Convert_USART_SendComplete(fd))
	{
		Timer_Restart((MTIMER*)&tm_FrmAckTo);
	}
	
	//Ack Timeout
	if(Timer_Expires((MTIMER*)&tm_FrmAckTo))
	{
		if(++comm_err_cnt > HReg[CP_FAULT_TIMES])
		{
			comm_err_cnt= HReg[CP_FAULT_TIMES];
		}
		
		if(ProtocolConvert->request_proc)
		{
		}
		else
		{
			Timer_Restart((MTIMER*)&tm_FrmAckTo);
		}
	}
	
	if(comm_err_cnt >= HReg[CP_FAULT_TIMES])
	{
		IReg[0]= 1;
	}
	else
	{
		IReg[0]= 0;
	}
	
	if(HReg[CP_PROTOCOL_ID]>= PROTOCOL_MAX)
	{
		HReg[CP_PROTOCOL_ID]= PROTOCOL_MAX;
	}
}

void record_uart_ptc_status(int status)
{
	char cTemp[128];
    int   fd; // , send_res;
    // 2. 打开PTC私有协议对应的串口
    fd = open("./uart_ptc_status_record", O_RDWR|O_NOCTTY/*|O_NDELAY*/);
    if (fd < 0) {
        return;
    }
	sprintf(cTemp, "%ld\t%d\r\n", time(NULL), status);
	write(fd, cTemp, strlen(cTemp));
	close(fd);
	
}


