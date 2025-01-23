
#ifndef __HCTM_H__
#define __HCTM_H__

#include <stdint.h>

void HCTM_Init(int fd, uint16_t addr);

//////////////////////////////////////////////////////
uint8_t HCTM_WCPC0703E_Analysis(uint16_t len);
uint16_t HCTM_WCPC0703E_DataColumnsNumber();
uint16_t HCTM_WCPC0703E_DataOutput(char* strOutput);

#endif


