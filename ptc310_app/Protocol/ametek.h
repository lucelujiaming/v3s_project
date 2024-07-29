
#ifndef __AMETEK_H__
#define __AMETEK_H__

#include <stdint.h>

void AMETEK_Init(int fd, uint16_t addr);

//////////////////////////////////////////////////////
uint16_t AMETEK_2850_Request(void);
uint8_t AMETEK_2850_Analysis(uint16_t len);

uint16_t AMETEK_5000_Request(void);
uint8_t AMETEK_5000_Analysis(uint16_t len);
#endif


