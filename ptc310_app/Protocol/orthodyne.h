
#ifndef __ORTHODYNE_H__
#define __ORTHODYNE_H__

#include <stdint.h>

void ORTHODYNE_Init(int fd, uint16_t addr);

//////////////////////////////////////////////////////
uint8_t ORTHODYNE_Analysis(uint16_t len);
#endif


