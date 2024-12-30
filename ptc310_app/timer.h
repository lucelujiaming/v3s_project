#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// 自定义定时器结构体
typedef struct
{
	char * name;
 	uint8_t timer_pos;         // 定时器序号
 	bool timer_repeat;         // 是否为循环定时器。
 	                           // true  - 循环定时器。
 	                           //         这种情况下，在超时以后，定时器继续工作。
 	                           // false - 非循环定时器。
 	                           //         这种情况下，在超时以后，定时器停止。
 	bool ticks_en;             // 定时器是否使能
 	// uint32_t ticks_ms;      // 定时器累积时间，单位为毫秒。
	struct timespec ticks_ms;  // 之前的数据类型为整数，现在需要修改为timespec类型。
	                           // 0 - 定时器停止。
	uint32_t ticks_limit;      // 定时器超时时间，单位为毫秒。
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
// 该函数已经废弃。
void Timer_TicksInc(void);
#endif
