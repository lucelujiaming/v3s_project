
#ifndef __TELEDYNE_H__
#define __TELEDYNE_H__

#include <stdint.h>

void TELEDYNE_Init(int fd, uint16_t addr);

//////////////////////////////////////////////////////
uint8_t TELEDYNE_Analysis(uint16_t len);
#endif


