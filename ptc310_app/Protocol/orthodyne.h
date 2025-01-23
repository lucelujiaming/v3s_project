
#ifndef __ORTHODYNE_H__
#define __ORTHODYNE_H__

#include <stdint.h>

void ORTHODYNE_Init(int fd, uint16_t addr);

//////////////////////////////////////////////////////
uint8_t ORTHODYNE_Analysis(uint16_t len);
uint16_t ORTHODYNE_DataColumnsNumber();
uint16_t ORTHODYNE_DataOutput(char* strOutput);

#endif


