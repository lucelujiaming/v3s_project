//Timer.c
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "timer.h"

#define TIMER_MAX	16
static MTIMER *TimerTable[TIMER_MAX]= {NULL}; 

void Timer_SetParam(MTIMER *timer, bool repeat,uint32_t limit)
{
	timer->timer_repeat= repeat;
	clock_gettime(CLOCK_MONOTONIC, &timer->ticks_ms);
	timer->ticks_limit= limit;
}

void Timer_Start(MTIMER *timer)
{
	timer->ticks_en= 1;
}

void Timer_Stop(MTIMER *timer)
{
	timer->ticks_ms.tv_sec  = 0;
	timer->ticks_ms.tv_nsec = 0;
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
	clock_gettime(CLOCK_MONOTONIC, &timer->ticks_ms);
	timer->ticks_en= 1;
}

bool Timer_Expires(MTIMER *timer)
{
	struct timespec current_ts;
	clock_gettime(CLOCK_MONOTONIC, &current_ts);

	int current_ms_result = current_ts.tv_sec * 1000 + current_ts.tv_nsec / 1000000 ;
	int ticks_ms_result = timer->ticks_ms.tv_sec * 1000 + timer->ticks_ms.tv_nsec / 1000000 ;
	int diff_ms_timespec = current_ms_result - ticks_ms_result;

	//  if(timer->ticks_en)
	//  {
	//  	printf("Timer_Expires (%d - %d) = %d with %d\n", 
	//  		current_ms_result, ticks_ms_result, diff_ms_timespec, timer->ticks_limit);
	//  }
	//  else 
	//  {
	//  	printf("Timer_Expires stops and timer->ticks_limit = %d\n", timer->ticks_limit);
	//  }
	
	// printf("Timer_Expires timer->ticks_ms.tv_sec = %ld\n", timer->ticks_ms.tv_sec);
	// printf("Timer_Expires timer->ticks_limit = %d\n", timer->ticks_limit);
	// printf("Timer_Expires current_ts.tv_sec = %ld and timer->ticks_ms.tv_sec = %ld \n", 
	//       current_ts.tv_sec, timer->ticks_ms.tv_sec);
	// printf("Timer_Expires current_ts.tv_nsec = %ld and timer->ticks_ms.tv_nsec = %ld \n", 
	//        current_ts.tv_nsec, timer->ticks_ms.tv_nsec);
	// printf("Timer_Expires diff_timespec.tv_nsec = %ld\n", 
	// 	  (current_ts.tv_nsec - timer->ticks_ms.tv_nsec) / 1000000);
	
	
	// 如果定时器使能，且定时器累积时间大于限制时间。说明定时器成功超时。
	if(timer->ticks_en && 
		ticks_ms_result &&
		(diff_ms_timespec >= timer->ticks_limit))
	{
		// 如果定时器不是循环定时器，
		if(timer->timer_repeat== 0)
		{
			// 在超时以后，需要停掉定时器。
			timer->ticks_en= 0;
			timer->ticks_ms.tv_sec  = 0;
			timer->ticks_ms.tv_nsec = 0;
			printf("Timer_Expires stops and timer->ticks_limit = %d\n", timer->ticks_limit);
		}
		// 否则如果定时器为循环定时器，则在超时以后，定时器需要继续工作。
		else {
			clock_gettime(CLOCK_MONOTONIC, &timer->ticks_ms);
			// printf("Timer_Expires continues and timer->ticks_limit = %d\n", timer->ticks_limit);
		}
		// timer->ticks_ms= time(NULL);
		// printf("Timer_Expires(%s) returns true and timer->ticks_limit = %d\n", timer->name, timer->ticks_limit);
		return true;
	}
	// 否则认为定时器没有超时。
	else
	{
		// printf("Timer_Expires(%s) returns false and timer->ticks_limit = %d, ticks_ms_result= %d, diff_ms_timespec = %d\n",
		//  	timer->name, timer->ticks_limit, ticks_ms_result, diff_ms_timespec);
		return false;
	}
	return false;
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
	printf("This timer out of range\n");
	return false;
}

// 该函数已经废弃。
void Timer_TicksInc(void)
{
	uint8_t i;
	
	for(i= 0;i<TIMER_MAX;i++)
	{
		if(TimerTable[i]!= NULL)
		{
			if(TimerTable[i]->ticks_en)
			{
			//  	if(TimerTable[i]->ticks_ms < TimerTable[i]->ticks_limit)
			//  	{
			//  		// TimerTable[i]->ticks_ms.tv_nsec++;
			//  	}
			}
			//      else
			//      {
			//      	TimerTable[i]->ticks_ms= 0;
			//      }
		}
	}
}
