
#ifndef __MEECO_H__
#define __MEECO_H__

#include <stdint.h>

void MEECO_Init(int fd, uint16_t addr);

//////////////////////////////////////////////////////
uint16_t MEECO_Request(void);
uint8_t MEECO_Analysis(uint16_t len);
uint16_t MEECO_DataColumnsNumber();
uint16_t MEECO_DataOutput(char* strOutput);

#endif


