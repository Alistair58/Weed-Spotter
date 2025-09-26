#include "pump.hpp"

#include <iostream>
#include <exception>
#include <unistd.h>
#include <pigpio.h>


void shootWeedKiller(int dutyCyclePercentage,float durationSeconds){
	int durationUs = durationSeconds * 1e6;
	//1KHz
	int frequency = 1000; 
	//0-1,000,000
	int dutyCycle = dutyCyclePercentage * 1e4;
	gpioHardwarePWM(PUMP_GPIO,frequency,dutyCycle);
	std::cout << "PWM on" << std::endl;

	gpioDelay(durationUs);
	gpioHardwarePWM(PUMP_GPIO,0,0);

	std::cout << "PWM off" << std::endl;
}
