#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct
{
	uint8_t timer_pos;
	bool timer_repeat;
	bool ticks_en;
	uint32_t ticks_ms;
	uint32_t ticks_limit;
}MTIMER;

/////////////////////////////////////////////////////////////////
void Timer_SetParam(MTIMER *timer,bool repeat,uint32_t limit);
void Timer_Start(MTIMER *timer);
void Timer_Stop(MTIMER *timer);
void Timer_Pause(MTIMER *timer);
void Timer_Resume(MTIMER *timer);
void Timer_Restart(MTIMER *timer);
bool Timer_Expires(MTIMER *timer);

/////////////////////////////////////////////////////////
bool Timer_Init(MTIMER *timer);
void Timer_TicksInc(void);
#endif
