#include "picoi2c.hpp"
#include <iostream>
#include <pigpio.h>
#include <unistd.h>
#include <cstring>
#include "globals.hpp"
#include "pump.hpp"
#include <fcntl.h>


void picoI2cListenBlocking(int pipefd[2]){
	bool weed = false;
	char weedX = 0;
	char weedY = 0;
	char buffer[3] = {0};
	//We don't write
	close(pipefd[1]);
	//Make read end of pipe non-blocking
	fcntl(pipefd[0],F_SETFL,O_NONBLOCK);

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
	const int shootDutyCycle = 100;
	const float shootDuration = 0.5f;
	while(true){
		int retVal = read(pipefd[0],buffer,sizeof(buffer));
		//std::cout << retVal << std::endl;
		if(retVal==sizeof(buffer)){
			std::cout << "Read pipe" << std::endl;
			weed = buffer[0] == 1;
			weedX = buffer[1];
			weedY = buffer[2];
		}
		int status = bscXfer(&xfer);
		//We got something
		if(status >= 0 && xfer.rxCnt > 0){
			std::cout << "Received "<< xfer.rxCnt << " bytes on I2C" << std::endl;
			if(xfer.rxCnt<2) continue;
			for(int i=0;i<(xfer.rxCnt-1);i++){
				printf("rxBuf[i]: %x rxBuf[i+1]: %x\n",xfer.rxBuf[i],xfer.rxBuf[i+1]);
				if(xfer.rxBuf[i] == ZERO_CHECK_BYTE){
					if(xfer.rxBuf[i+1] == POWEROFF){
						std::cout << "Powering off" << std::endl;
						//Stop I2C
						xfer.control = 0;
						xfer.txCnt = 0;
						bscXfer(&xfer);
						gpioTerminate();
						system("poweroff");
						std::cerr << "Could not power off" << std::endl;
					}
					if(xfer.rxBuf[i+1] == HAS_WEED){
						printf("weed: %d weedY: %f weedY: %f\n",weed?1:0,weedX,weedY);
						char reply[] = {(char) (weed?1:0),weedX,weedY};
						memcpy(xfer.txBuf,reply,sizeof(reply));
						xfer.txCnt = sizeof(reply);
						printf("Sent: {%d,%d,%d}\n",reply[0],reply[1],reply[2]);
						bscXfer(&xfer);
					}
					if(xfer.rxBuf[i+1]==SHOOT){
						std::cout << "Shooting" << std::endl;
						//Reply
						xfer.control = 0;
						xfer.txCnt = 0;
						bscXfer(&xfer);
						shootWeedKiller(shootDutyCycle,shootDuration);
					}
				}
			}
		}
		usleep(polling_period_micro);
	}
	close(pipefd[0]);
	//Stop I2C
	xfer.control = 0;
	bscXfer(&xfer);
	gpioTerminate();
}
