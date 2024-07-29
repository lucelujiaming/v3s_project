
#ifndef __TIGER_H__
#define __TIGER_H__

#include <stdint.h>

void TIGER_Init(int fd, uint16_t addr);

//////////////////////////////////////////////////////
uint16_t TIGER_Request(void);
uint8_t TIGER_Analysis(uint16_t len);
#endif


