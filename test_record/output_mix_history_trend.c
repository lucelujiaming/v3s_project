#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>

#include <string.h>

#define    TIME_SPAN               900 //   15mins   600   // 10 minutes
// #define    NEW_YEAR_DAY_2024    1704038400

#define  INSTRUMENT_ONLINE             0
#define  BATTERY_OFFLINE               1
#define  USART_OFFLINE                 2
#define  INSTRUMENT_STATUS             3

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

    sprintf(cFileName, "instrument_history_record_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    append_file(cFileName, cFileContent);
	
    struct tm*     tmToday  = localtime(&timeNow);
    tmToday->tm_hour = tmToday->tm_min = tmToday->tm_sec = 0;
    time_t timeToday = mktime(tmToday);
    
    // sprintf(cFileContent, "[%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d],\r\n",
    sprintf(cFileContent, "[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d],\r\n",
            // timeNow - timeToday,
            iParticleFirst, iParticleSecond, iParticleThird, iParticleFourth, 
            iParticleFifth, iParticleSixth, iParticleSeventh, iParticleEighth, 
            iLaseroRefFirst, iLaserRefSecond);

    sprintf(cFileName, "instrument_history_info_record_unixtime_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    printf("We output the instrument_history_info_record_unixtime_%04d_%02d_%02d.txt at %ld.\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, timeNow);
    append_file(cFileName, cFileContent);

}

void out_battery_info_record(int iOfflineStatus, time_t iFakeTimeStamp)
{
    static int iLastOfflineStatus = INSTRUMENT_STATUS;
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

    sprintf(cFileContent, "[\"%04d-%02d-%02d %02d:%02d:%02d\",%d],\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, 
            tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec, iOfflineStatus);

    sprintf(cFileName, "battery_info_record_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    append_file(cFileName, cFileContent);
    
    if(iLastOfflineStatus != iOfflineStatus)
    {
        iLastOfflineStatus = iOfflineStatus;
        
        sprintf(cFileName, "battery_info_switch_record_%04d_%02d_%02d.txt", 
                tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
        append_file(cFileName, cFileContent);
    }
	
    struct tm*     tmToday  = localtime(&timeNow);
    tmToday->tm_hour = tmToday->tm_min = tmToday->tm_sec = 0;
    time_t timeToday = mktime(tmToday);
	
    // sprintf(cFileContent, "[%ld,%d],\r\n",
    //        timeNow - timeToday, iOfflineStatus);
    sprintf(cFileContent, "%d,\r\n", iOfflineStatus);

    sprintf(cFileName, "battery_info_record_unixtime_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    printf("We output the battery_info_record_unixtime_%04d_%02d_%02d.txt at %ld.\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, timeNow);
    append_file(cFileName, cFileContent);
}

void out_usart_info_record(int iOfflineStatus, time_t iFakeTimeStamp)
{
    static int iLastOfflineStatus = INSTRUMENT_STATUS;
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

    sprintf(cFileContent, "[\"%04d-%02d-%02d %02d:%02d:%02d\",%d],\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, 
            tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec, iOfflineStatus);

    sprintf(cFileName, "usart_info_record_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    append_file(cFileName, cFileContent);
	
    if(iLastOfflineStatus != iOfflineStatus)
    {
        iLastOfflineStatus = iOfflineStatus;
        
        sprintf(cFileName, "usart_info_switch_record_%04d_%02d_%02d.txt", 
                tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
        append_file(cFileName, cFileContent);
    }
    
    struct tm*     tmToday  = localtime(&timeNow);
    tmToday->tm_hour = tmToday->tm_min = tmToday->tm_sec = 0;
    time_t timeToday = mktime(tmToday);
    
    // sprintf(cFileContent, "[%ld,%d],\r\n",
    //        timeNow - timeToday, iOfflineStatus);
    sprintf(cFileContent, "%d,\r\n", iOfflineStatus);

    sprintf(cFileName, "usart_info_record_unixtime_%04d_%02d_%02d.txt", 
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
    printf("We output the usart_info_record_unixtime_%04d_%02d_%02d.txt at %ld.\r\n",
            tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday, timeNow);
    append_file(cFileName, cFileContent);
}

int test_mix_history_record()
{
    time_t timeNow = time(NULL);

    int iParticleFirst   = 0;   int iParticleSecond  = 0; 
    int iParticleThird   = 0;   int iParticleFourth  = 0; 
    int iParticleFifth   = 0;   int iParticleSixth   = 0;  
    int iParticleSeventh = 0;   int iParticleEighth  = 0; 
    int iLaseroRefFirst  = 0;   int iLaserRefSecond  = 0; 

    int iUsartOfflineStatus   = INSTRUMENT_ONLINE;
    int iBatteryOfflineStatus = INSTRUMENT_ONLINE;
    
    // Start from midnight.
    struct tm*     tmToday  = localtime(&timeNow);
    tmToday->tm_hour = tmToday->tm_min = tmToday->tm_sec = 0;
    time_t timeToday = mktime(tmToday);
    
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
            iLaseroRefFirst, iLaserRefSecond,  timeToday + TIME_SPAN * i);
        
        if(((i / 3) * 3 == i) && (i != 0))
        {
            if(iUsartOfflineStatus == INSTRUMENT_ONLINE)
            {
                 iUsartOfflineStatus = USART_OFFLINE;
            }
            else 
            {
                 iUsartOfflineStatus = INSTRUMENT_ONLINE;
            }
        }
        if(((i / 7) * 7 == i) && (i != 0))
        {
            if(iBatteryOfflineStatus == INSTRUMENT_ONLINE)
            {
                 iBatteryOfflineStatus = BATTERY_OFFLINE;
            }
            else 
            {
                 iBatteryOfflineStatus = INSTRUMENT_ONLINE;
            }
        }
        out_battery_info_record(iBatteryOfflineStatus, timeToday + TIME_SPAN * i);
        out_usart_info_record(iUsartOfflineStatus, timeToday + TIME_SPAN * i);
    }
}


int main(int argc, char ** argv)
{
    test_mix_history_record();
    return 1;
}
