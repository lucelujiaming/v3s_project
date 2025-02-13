#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <sys/poll.h>
#include <time.h>
#include <stdint.h> // for uint64_t

#include <pthread.h>

int create_timer_fd(int iStartSecond, int iInterval)
{
    int timer_fd;
    struct itimerspec new_timer;

    // 创建一个基于单调时钟的定时器
    timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer_fd == -1) {
        perror("timerfd_create failed");
        exit(EXIT_FAILURE);
    }

    // 设置定时器：2秒后触发，然后每隔1秒触发一次
    new_timer.it_value.tv_sec = iStartSecond;  // 首次触发的时间（秒）
    new_timer.it_value.tv_nsec = 0;            // 首次触发的时间（纳秒）
    new_timer.it_interval.tv_sec = iInterval;  // 每次触发的间隔时间（秒）
    new_timer.it_interval.tv_nsec = 0;         // 每次触发的间隔时间（纳秒）

    if (timerfd_settime(timer_fd, 0, &new_timer, NULL) == -1) {
        perror("timerfd_settime failed");
        exit(EXIT_FAILURE);
    }
    printf("Timer started! It will fire in %d seconds, then every %d second.\n", iStartSecond, iInterval);
    return timer_fd;
}

static void* first_timer_operation(void *arg)
{
    int first_timer_fd = 0; 
    first_timer_fd = create_timer_fd(2, 1);

    // 使用 poll 监视定时器文件描述符
    struct pollfd poll_fds;
    poll_fds.fd = first_timer_fd;
    poll_fds.events = POLLIN; // 等待定时器可读事件

    while (1) {
        int ret = poll(&poll_fds, 1, -1); // 无限等待
        if (ret == -1) {
            perror("poll failed");
            exit(EXIT_FAILURE);
        }

        if (poll_fds.revents & POLLIN) {
            uint64_t expirations;
            ssize_t s = read(first_timer_fd, &expirations, sizeof(expirations));
            if (s != sizeof(expirations)) {
                perror("read failed");
                exit(EXIT_FAILURE);
            }

            printf("first_timer_operation Timer expired %llu times\n", (unsigned long long) expirations);
        }
    }

    close(first_timer_fd);
}

static void* second_timer_operation(void *arg)
{
    int second_timer_fd = 0; 
    second_timer_fd = create_timer_fd(3, 2);

    // 使用 poll 监视定时器文件描述符
    struct pollfd poll_fds;
    poll_fds.fd = second_timer_fd;
    poll_fds.events = POLLIN; // 等待定时器可读事件

    while (1) {
        int ret = poll(&poll_fds, 1, -1); // 无限等待
        if (ret == -1) {
            perror("poll failed");
            exit(EXIT_FAILURE);
        }

        if (poll_fds.revents & POLLIN) {
            uint64_t expirations;
            ssize_t s = read(second_timer_fd, &expirations, sizeof(expirations));
            if (s != sizeof(expirations)) {
                perror("read failed");
                exit(EXIT_FAILURE);
            }

            printf("second_timer_operation Timer expired %llu times\n", (unsigned long long) expirations);
        }
    }

    close(second_timer_fd);
}

int main() {
    
    pthread_t first_timer_thread;
    pthread_t second_timer_thread;
    
    
	pthread_create(&first_timer_thread, NULL, first_timer_operation, NULL);
	// printf("Start Protocol_Proc and ret return %d\n", ret);
	pthread_create(&second_timer_thread, NULL, second_timer_operation, NULL);
    while(1);
    
    return 0;
}


