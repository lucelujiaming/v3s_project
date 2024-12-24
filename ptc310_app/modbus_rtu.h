#ifndef __MODBUS_RTU_H__
#define __MODBUS_RTU_H__


#include <termios.h>

int modbus_rtu_connect(const char *device,
           int baud, char parity, int data_bit, int stop_bit, struct termios* old_tios);


#endif

