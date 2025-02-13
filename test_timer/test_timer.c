#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include <time.h>

void timer_handler(int signum) {
    // 执行定时器到期时需要做的操作
    // 注意：定时器处理函数应该尽量保持简短，避免执行耗时操作
    printf("timed\n");
}

void set_timer(int seconds) {
    struct itimerval timer;
    timer.it_value.tv_sec = seconds;  // 第一次定时器到期的秒数
    timer.it_value.tv_usec = 0;       // 第一次定时器到期的微秒数
    // timer.it_interval = timer.it_value;
    timer.it_interval.tv_sec = seconds;  // 定时器周期的秒数（如果为0，则只执行一次）
    timer.it_interval.tv_usec = 0;       // 定时器周期的微秒数

    // 设置定时器信号处理函数
    // signal(SIGALRM, timer_handler);
    struct sigaction act;
    act.sa_handler = timer_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask); 
    sigaction(SIGALRM,&act,NULL); //设置信号 SIGALRM 的处理函数为 timer_handler

    // 启动定时器
    setitimer(ITIMER_REAL, &timer, NULL);
}

/*停止setitimer定时器*/
void delete_setitimer() 
{
    struct itimerval value; 
    value.it_value.tv_sec = 0; 
    value.it_value.tv_usec = 0; 
    value.it_interval = value.it_value; 
    setitimer(ITIMER_REAL, &value, NULL); 
}

int main()
{
    set_timer(2);

    while(1);
    return 0;
}


