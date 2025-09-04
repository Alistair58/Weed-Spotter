#include "picoi2c.hpp"

//TODO
//the bit banging version doesn't exist, I need to resolder

void picoI2cListenBlocking(void){
	if(gpioInitialise() < 0){
		throw std::runtime_error("Could not initialise GPIO pins");
	}
	//pigpio will bit bang on GP2 and GP3
	int handle = i2cSlave(ZERO_I2C_ADDR);
	if(handle < 0){
		throw std::runtime_error("Could not set up i2c as a slave");
	}
	//we then check the buffer periodically
	//The user presses disconnect, the pico sends us poweroff and then we poweroff
	//The user can't press disconnect and then flip the off switch within 0.1s
	useconds_t polling_period_micro = 1e5;
	while(true){
		//The poweroff message is the only valid message as of 04/09/25
		//It consist of {ZERO_CHECK_BYTE,POWEROFF}
		uint8_t buffer[2] = {0};
		int bytes = i2cReadDevice(handle,buffer,sizeof(buffer));
		//We got something
		if(bytes > 0){
			std::cout << "Received "<< bytes " bytes on i2c" << std::endl;
			if(bytes!=2) continue;
			if(buffer[0] == ZERO_CHECK_BYTE && buffer[1] == POWEROFF){
				std::cout << "Powering off" << std::endl;
				execl("poweroff","poweroff");
				std::cerr << "Could not power off" << std::endl;
			}
		}
		usleep(polling_period_micro);
	}
}
