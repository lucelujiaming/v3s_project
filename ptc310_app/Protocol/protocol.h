
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stdint.h>
#include <pthread.h>
#include "mbvardef.h"

#define DATA_LENGTH    256

#define PROTOCOL_MAX		18	
//
#define PT_PMS_PDS_PA				0
#define PT_PMS_MLPC101_HP		1
#define PT_PMS_LASAIR_III		2

#define PT_MEECO				3			//Tracer 2, Aquavolt

#define PT_JAG					4			//MX1-A, MXNY

#define PT_PEAK					5			//FID, RCP, PHDID

#define PT_DELTA_F			6			//DF-310E, DF-550E, DF-560E, DF-745, DF-750, DF-760E

#define PT_TIGER				7			//HALO+, HALO 3Q, SPARK

#define PT_ORTHODYNE		8			//DID550, ORTHOSMART

#define PT_SAES					9		//TA7000F, TA5000F
#define PT_AMETEK_5000		10	//5380, 5920
#define PT_AMETEK_2850		11

#define PT_TELEDYNE				12	//U3000

#define PT_SERVOMEX_NANOCHROME		13
#define PT_SERVOMEX_K1000A				14
#define PT_HCTM_WCPC0703E					15
#define PT_PMS_PDS_E							16	//PMS Mode
#define PT_RELIYA_HGPC_100				17

typedef enum {
    INSTRUMENT_STATUS_UNKNOWN = 0,   // Not used
    BATTERY_ONLINE            = 1,
    BATTERY_OFFLINE           = 2,
    MODBUS_USART_ONLINE       = 3,
    MODBUS_USART_OFFLINE      = 4,
    INSTRUMENT_USART_ONLINE   = 5,
    INSTRUMENT_USART_OFFLINE  = 6,
    INSTRUMENT_STATUS_MAX     = 7, 
} PTC310_INTERFACE_STATUS;
	
#define MODBUS_USART_OFFLINE_TO				10


void Protocol_Init(int fd);
uint16_t Protocol_DataOutput(char * strOutput);
uint16_t Protocol_GetDataColumnsNumber();

void out_modbus_usart_info_record(PTC310_INTERFACE_STATUS iOfflineStatus, time_t iFakeTimeStamp);
void out_instrument_usart_info_record(PTC310_INTERFACE_STATUS iOfflineStatus, time_t iFakeTimeStamp);
void check_instrument_usart_info_record(PTC310_INTERFACE_STATUS iOfflineStatus);

void Protocol_Proc(int fd);
// void Update_Protocol_Config();

extern int16_t HReg[HREG_MAX];
extern int16_t IReg[IREG_MAX];

// 2. 定义全局变量和读写锁
extern pthread_rwlock_t hreg_rwlock;
extern pthread_rwlock_t ireg_rwlock;

extern char data_temp[100];
extern int16_t* protocol_buff;
extern uint8_t* serial_buff;
extern uint8_t little_endian;

extern PTC310_INTERFACE_STATUS g_iInstrumentUsartOfflineStatus;

#endif


