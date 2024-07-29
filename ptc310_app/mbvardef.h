#ifndef __MODBUS_VAR_DEF_H__
#define __MODBUS_VAR_DEF_H__

#define FIRMWARE_VERSION	134

#define IREG_MAX	200
#define HREG_MAX	10

////////////////////////////////////////////////
#define CP_EEP_BASE			0 
#define CP_EEP_MAX			8

#define CP_PROTOCOL_ID				0
#define CP_INSTRUMENT_ADDR		1
#define CP_ENQUIRY_TIME				2		//0.1s
#define CP_RESPONSE_TIME			3		//0.1s
#define CP_32BIT_LE						4		//Little Endian
#define CP_FAULT_TIMES				5

#define HR_SW_VERSION					8
#define HR_UNIT_RESET					9
#endif
