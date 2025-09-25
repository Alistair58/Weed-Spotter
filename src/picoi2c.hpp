#ifndef PICOI2C_HPP
#define PICOI2C_HPP

//Names match the code on the Pico
//7 bit address
#define ZERO_I2C_ADDR 0x2A
#define ZERO_CHECK_BYTE 0xA6

typedef enum ZERO_CMDS{
	POWEROFF = 0x10
} ZERO_CMDS;

void picoI2cListenBlocking(void);

#endif
