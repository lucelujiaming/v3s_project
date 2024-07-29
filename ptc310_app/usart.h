#ifndef __SERIAL_H
#define __SERIAL_H

#include <sys/types.h>
 
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define SERIAL_FALSE  -1
#define SERIAL_TRUE   0

void set_speed(int fd, int speed);
int set_parity(int fd,int databits,int stopbits,int parity);

#endif
