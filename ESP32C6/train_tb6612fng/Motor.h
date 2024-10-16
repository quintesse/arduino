#ifndef MOTOR_h
#define MOTOR_h

#include <Arduino.h>

class Motor
{
  public:
    // Constructor. Mainly sets up pins.
    Motor(int In1pin, int In2pin, int PWMpin, int offset, int STBYpin);      

    // Drive in direction given by sign, at speed given by magnitude of the parameter.
    void drive(int speed);  
	
	  //Stops motor by setting both input pins high
    void brake(); 
	
	  //set the chip to standby mode.  The drive function takes it out of standby 
  	void standby();	
	
  private:
    //variables for the 2 inputs, PWM input, Offset value, and the Standby pin
  	int In1, In2, PWM, Offset, Standby;	
};

#endif
