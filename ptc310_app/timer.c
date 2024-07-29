//Timer.c
#include <stdbool.h>
#include "timer.h"

#define TIMER_MAX	16
static MTIMER *TimerTable[TIMER_MAX]= {NULL}; 

void Timer_SetParam(MTIMER *timer, bool repeat,uint32_t limit)
{
	timer->timer_repeat= repeat;
	timer->ticks_ms= 0;
	timer->ticks_limit= limit;
}

void Timer_Start(MTIMER *timer)
{
	timer->ticks_en= 1;
}

void Timer_Stop(MTIMER *timer)
{
	timer->ticks_ms= 0;
	timer->ticks_en= 0;
}

void Timer_Pause(MTIMER *timer)
{
	timer->ticks_en= 0;
}

void Timer_Resume(MTIMER *timer)
{
	timer->ticks_en= 1;
}

void Timer_Restart(MTIMER *timer)
{
	timer->ticks_ms= 0;
	timer->ticks_en= 1;
}

bool Timer_Expires(MTIMER *timer)
{
	if(timer->ticks_en && (timer->ticks_ms>= timer->ticks_limit))
	{
		if(timer->timer_repeat== 0)
		{
			timer->ticks_en= 0;
		}
		timer->ticks_ms= 0;
		return true;
	}
	else
	{
		return false;
	}
}

/////////////////////////////////////////////////////////
bool Timer_Init(MTIMER *timer)
{
	uint8_t i;
	
	for(i= 0;i<TIMER_MAX;i++)
	{
		if(TimerTable[i]== NULL)
		{
			TimerTable[i]= timer;
			timer->timer_pos= i;
			return true;
		}
	}
	return false;
}

void Timer_TicksInc(void)
{
	uint8_t i;
	
	for(i= 0;i<TIMER_MAX;i++)
	{
		if(TimerTable[i]!= NULL)
		{
			if(TimerTable[i]->ticks_en)
			{
				if(TimerTable[i]->ticks_ms< TimerTable[i]->ticks_limit)
				{
					TimerTable[i]->ticks_ms++;
				}
			}
			else
			{
				TimerTable[i]->ticks_ms= 0;
			}
		}
	}
}
