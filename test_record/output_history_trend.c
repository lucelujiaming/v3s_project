#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>

#include <string.h>

// #define    NEW_YEAR_DAY_2024    1704038400

int append_file(char * cFileName, char * cFileContent)
{
    int   fd; // , send_res;
    // 2. 打开PTC私有协议对应的串口
    fd = open(cFileName, O_RDWR | O_APPEND);
    if (fd < 0) {
        fd = open(cFileName, O_RDWR | O_CREAT);
        if (fd < 0) {
            return -1;
        }
    }
	write(fd, cFileContent, strlen(cFileContent));
	close(fd);
    return 0;
}

void out_instrument_history_record(
        int iParticleFirst,   int iParticleSecond, 
        int iParticleThird,   int iParticleFourth, 
        int iParticleFifth,   int iParticleSixth,  
        int iParticleSeventh, int iParticleEighth, 
        int iLaseroRefFirst,  int iLaserRefSecond, time_t iFakeTimeStamp)
{
    char cFileName[128];
    char cFileContent[1024];

    time_t timeNow;
    if(iFakeTimeStamp == 0)
    {
        timeNow = time(NULL);
    }
    else 
    {
        timeNow = iFakeTimeStamp;
    }
    struct tm*     tmNow    = localtime(&timeNow);

    sprintf(cFileContent, "\"%04d-%02d-%02d %02d:%02d:%02d\",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, 
            tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec,
            iParticleFirst, iParticleSecond, iParticleThird, iParticleFourth, 
            iParticleFifth, iParticleSixth, iParticleSeventh, iParticleEighth, 
            iLaseroRefFirst, iLaserRefSecond);

    sprintf(cFileName, "instrument_history_record_%04d_%02d_%02d.data", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    append_file(cFileName, cFileContent);
	
    struct tm*     tmToday  = localtime(&timeNow);
    tmNow->tm_hour = tmNow->tm_min = tmNow->tm_sec = 0;
    time_t timeToday = mktime(tmToday);
    
    sprintf(cFileContent, "[%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d],\r\n",
            timeNow - timeToday,
            iParticleFirst, iParticleSecond, iParticleThird, iParticleFourth, 
            iParticleFifth, iParticleSixth, iParticleSeventh, iParticleEighth, 
            iLaseroRefFirst, iLaserRefSecond);

    sprintf(cFileName, "instrument_history_info_record_unixtime_%04d_%02d_%02d.data", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    append_file(cFileName, cFileContent);

}

int test_instrument_history_record()
{
    time_t timeNow = time(NULL);

    int iParticleFirst   = 0;   int iParticleSecond  = 0; 
    int iParticleThird   = 0;   int iParticleFourth  = 0; 
    int iParticleFifth   = 0;   int iParticleSixth   = 0;  
    int iParticleSeventh = 0;   int iParticleEighth  = 0; 
    int iLaseroRefFirst  = 0;   int iLaserRefSecond  = 0; 

    srand((unsigned int)time(NULL));
    for(int i = 0; i < 100; i++)
    {
        iParticleFirst   = rand() % 100;
        iParticleSecond  = rand() % 100; 
        iParticleThird   = rand() % 100;
        iParticleFourth  = rand() % 100; 
        iParticleFifth   = rand() % 100;;
        iParticleSixth   = rand() % 100; 
        iParticleSeventh = rand() % 100;
        iParticleEighth  = rand() % 100; 
        iLaseroRefFirst  = rand() % 100;
        iLaserRefSecond  = rand() % 100;
        out_instrument_history_record(
            iParticleFirst,   iParticleSecond, iParticleThird, 
            iParticleFourth,  iParticleFifth,  iParticleSixth, 
            iParticleSeventh, iParticleEighth, 
            iLaseroRefFirst, iLaserRefSecond,  timeNow + 100 * i);
    }
}

void out_battery_info_record(int iOfflineStatus, time_t iFakeTimeStamp)
{
    char cFileName[128];
    char cFileContent[1024];

    time_t timeNow;
    if(iFakeTimeStamp == 0)
    {
        timeNow = time(NULL);
    }
    else 
    {
        timeNow = iFakeTimeStamp;
    }
    struct tm*     tmNow  = localtime(&timeNow);

    sprintf(cFileContent, "\"%04d-%02d-%02d %02d:%02d:%02d\",%d\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, 
            tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec, iOfflineStatus);

    sprintf(cFileName, "battery_info_record_%04d_%02d_%02d.data", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    append_file(cFileName, cFileContent);
	
    struct tm*     tmToday  = localtime(&timeNow);
    tmNow->tm_hour = tmNow->tm_min = tmNow->tm_sec = 0;
    time_t timeToday = mktime(tmToday);
	
    sprintf(cFileContent, "[%ld,%d],\r\n",
            timeNow - timeToday, iOfflineStatus);

    sprintf(cFileName, "battery_info_record_unixtime_%04d_%02d_%02d.data", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    append_file(cFileName, cFileContent);
}

#define  BATTERY_OFFLINE     0
#define  BATTERY_ONLINE      1

#define  USART_OFFLINE       2
#define  USART_ONLINE        3

int test_battery_info_record()
{
    time_t timeNow = time(NULL);

    int iOfflineStatus = BATTERY_OFFLINE;

    srand((unsigned int)time(NULL));
    for(int i = 0; i < 100; i++)
    {
        if(iOfflineStatus == BATTERY_OFFLINE)
        {
             iOfflineStatus = BATTERY_ONLINE;
        }
        else 
        {
             iOfflineStatus = BATTERY_OFFLINE;
        }
        out_battery_info_record(iOfflineStatus, timeNow + 3000 * i);
    }
}

void out_usart_info_record(int iOfflineStatus, time_t iFakeTimeStamp)
{
    char cFileName[128];
    char cFileContent[1024];

    time_t timeNow;
    if(iFakeTimeStamp == 0)
    {
        timeNow = time(NULL);
    }
    else 
    {
        timeNow = iFakeTimeStamp;
    }
    struct tm*     tmNow  = localtime(&timeNow);

    sprintf(cFileContent, "\"%04d-%02d-%02d %02d:%02d:%02d\",%d\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, 
            tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec, iOfflineStatus);

    sprintf(cFileName, "usart_info_record_%04d_%02d_%02d.data", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    append_file(cFileName, cFileContent);
	
    struct tm*     tmToday  = localtime(&timeNow);
    tmNow->tm_hour = tmNow->tm_min = tmNow->tm_sec = 0;
    time_t timeToday = mktime(tmToday);
    
    sprintf(cFileContent, "[%ld,%d],\r\n",
            timeNow - timeToday, iOfflineStatus);

    sprintf(cFileName, "usart_info_record_unixtime_%04d_%02d_%02d.data", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    append_file(cFileName, cFileContent);
}

int test_usart_info_record()
{
    time_t timeNow = time(NULL);

    int iOfflineStatus = USART_OFFLINE;

    srand((unsigned int)time(NULL));
    for(int i = 0; i < 300; i++)
    {
        if(iOfflineStatus == USART_OFFLINE)
        {
             iOfflineStatus = USART_ONLINE;
        }
        else 
        {
             iOfflineStatus = USART_OFFLINE;
        }
        out_usart_info_record(iOfflineStatus, timeNow + 1000 * i);
    }
}



int main(int argc, char ** argv)
{
    test_instrument_history_record();
    test_battery_info_record();
    test_usart_info_record();
    return 1;
}
