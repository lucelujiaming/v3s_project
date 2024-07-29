#ifndef __MODBUS_CRC_H__
#define __MODBUS_CRC_H__

#include <stdint.h>

uint16_t _crc16_modbus(uint8_t *pdata, uint8_t size);

#endif
