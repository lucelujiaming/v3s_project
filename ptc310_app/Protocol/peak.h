
#ifndef __PEAK_H__
#define __PEAK_H__

#include <stdint.h>

void PEAK_Init(int fd, uint16_t addr);

//////////////////////////////////////////////////////
uint16_t PEAK_Request(void);
uint8_t PEAK_Analysis(uint16_t len);
#endif


