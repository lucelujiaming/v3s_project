
#ifndef __DELTA_F_H__
#define __DELTA_F_H__

#include <stdint.h>

void DELTAF_Init(int fd, uint16_t addr);

//////////////////////////////////////////////////////
uint16_t DELTAF_Request(void);
uint8_t DELTAF_Analysis(uint16_t len);
#endif


