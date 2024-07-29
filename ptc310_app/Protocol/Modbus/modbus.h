#ifndef __MODBUS_H__
#define __MODBUS_H__

#define MODBUS_BROADCAST_ADDR	255
//define exception code
#define ILLEGAL_FUNCTION	0x01
#define ILLEGAL_DATA_ADDR	0x02
#define ILLEGAL_DATA_VALUE	0x03
#define S_DEVICE_FAILURE	0x04
#define ACK					0x05
#define S_DEVICE_BUSY		0x06
#define NACK				0x07
#define MEMORY_PARITY_ERROR	0x08

//define function code
#define CMD_READ_COIL_STATUS		0x01
#define CMD_READ_INPUT_STATUS		0x02
#define CMD_READ_HOLDING_REGISTER	0x03
#define CMD_READ_INPUT_REGISTER		0x04
#define CMD_FORCE_SINGLE_COIL		0x05
#define CMD_PRESET_SINGLE_REGISTER  0x06
#define CMD_READ_EXCEPTION_STATUS	0x07
#define CMD_DIAGNOSTICS				0x08

#define CMD_FETCH_COMM_EVENT_COUNTER	0x0B
#define CMD_FETCH_COMM_EVENT_LOG		0x0C
#define CMD_PROGRAM_CONTROLLER			0x0D
#define CMD_POLL_CONTROLLER				0x0E
#define CMD_FORCE_MULTIPLE_COILS		0x0F
#define CMD_PRESET_MULTIPLE_REGISTERS	0x10
#define CMD_REPORT_SLAVE_ID				0x11

#define CMD_READ_GENERAL_REFERENCE		0x14
#define CMD_WRITE_GENERAL_REFERENCE		0x15
#define CMD_MASK_WRITE_REGISTER			0x16
#define CMD_MASK_READ_REGISTER			0x17

//User define command
#define CMD_READ_ADDRESS				0x41
#define CMD_WRITE_ADDRESS				0x42
#define CMD_READ_CONFIG					0x43
#define CMD_WRITE_CONFIG				0x44

#include "common.h"
#include "ModbusCrc.h"


#endif
