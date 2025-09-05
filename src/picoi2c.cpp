#include "picoi2c.hpp"

void picoI2cListenBlocking(void){
	if(gpioInitialise() < 0){
		throw std::runtime_error("Could not initialise GPIO pins");
	}
	bsc_xfer_t xfer;
	//23 bit number in little endian
	//bit 0  -> enable BSC peripheral
	//bit 2 -> enable I2C mode
	//bit 8 -> enable transmit
	//bit 9 -> enable receive
	//bits [16:22] -> address
	memset(&xfer,0,sizeof(bsc_xfer_t));
	xfer.control = (ZERO_I2C_ADDR<<16) | 0x305;

	if(bscXfer(&xfer) < 0){
		gpioTerminate();
		throw std::runtime_error("Could not set up i2c as a slave");
	}
	std::cout << "I2C slave running at address " << ZERO_I2C_ADDR << std::endl;
	//we then check the buffer periodically
	//The user presses disconnect, the pico sends us poweroff and then we poweroff
	//The user can't press disconnect and then flip the off switch within 0.1s
	useconds_t polling_period_micro = 1e5;
	while(true){
		//The poweroff message is the only valid message as of 04/09/25
		//It consist of {ZERO_CHECK_BYTE,POWEROFF}
		int status = bscXfer(&xfer);
		//We got something
		if(status >= 0 && xfer.rxCnt > 0){
			std::cout << "Received "<< xfer.rxCnt << " bytes on I2C" << std::endl;
			if(xfer.rxCnt<2) continue;
			for(int i=0;i<(xfer.rxCnt-1);i++){
				printf("\nrxBuf[i]: %x rxBuf[i+1]: %x",xfer.rxBuf[i],xfer.rxBuf[i+1]);
				if(xfer.rxBuf[i] == ZERO_CHECK_BYTE && xfer.rxBuf[i+1] == POWEROFF){
					std::cout << "Powering off" << std::endl;
					//Stop I2C
					xfer.control = 0;
					bscXfer(&xfer);
					gpioTerminate();
					system("poweroff");
					std::cerr << "Could not power off" << std::endl;
				}
			}
		}
		usleep(polling_period_micro);
	}
	//Stop I2C
	xfer.control = 0;
	bscXfer(&xfer);
	gpioTerminate();
}
