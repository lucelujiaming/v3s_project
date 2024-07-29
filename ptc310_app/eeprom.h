/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __EEPROM_H
#define __EEPROM_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "mbvardef.h"

/* Variables' number */
#define NumbOfVar               ((uint8_t)8)

uint16_t EE_Init(void);
uint16_t EE_ReadVariable(uint16_t VirtAddress, uint16_t* Data);
uint16_t EE_WriteVariable(uint16_t VirtAddress, uint16_t Data);

#endif /* __EEPROM_H */

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
