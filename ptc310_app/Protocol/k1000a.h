
#ifndef __SERVOMEX_K1000A_H__
#define __SERVOMEX_K1000A_H__

#include <stdint.h>

void SERVOMEX_K1000A_Init(int fd, uint16_t addr);

//////////////////////////////////////////////////////
uint8_t SERVOMEX_K1000A_Analysis(uint16_t len);
#endif


