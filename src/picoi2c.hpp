#ifndef PICOI2C_HPP
#define PICOI2C_HPP

#include <iostream>
#include <pigpio.h>
#include <unistd.h>
#include <cstring>

//Names match the code on the Pico
#define ZERO_I2C_ADDR 0xA0
#define ZERO_CHECK_BYTE 0xA6

typedef enum ZERO_CMDS{
	POWEROFF = 0x10
} ZERO_CMDS;

void picoI2cListenBlocking(void);

#endif
