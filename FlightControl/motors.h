#ifndef MOTORS_H
#define MOTORS_H

#include <stdint.h>
#include "../BlackLib/v3_0/BlackPWM/BlackPWM.h"

//motor control. 
//when this object is initialized, it has the ability to control any of our motors
//50HZ PWM signal
//PWM control through BlackLib

class motor_control{
	public:
		//Constructor
		//all time is measured in micro-seconds --- Greek leter mu, abbreviated u
		//initial duty cycle -- different between levitiation and stability
		//arm duty cycle -- switch to this duty cycle from initial to arm the motor
		//low duty cycle -- the lowest the motor can go 
		//high duty cycle -- highest the motor can go 
		motor_control(enum BlackLib::pwmName pwm_pin, double initial_us, double arm_us, double low_us, double high_us);

		//Raise PWM pulse by x%.  100% == high_us, 0% = low_us
		//Do not confuse this with raising/lower the Duty Cycle by a %
		//This is the way of traversing between low_us and high_us,
		//since direct control of the microseconds is not suggested.
		void raise_by_percent(double percent); 
		void lower_by_percent(double percent);

		//Get percentage
		double get_percent();
		//Get microseconds
		double get_microseconds();

		//will arm the motor if not already armed
		void arm();
	
	
		//will set the motor on low power
		void set_low();

		BlackLib::BlackPWM * tmp_testing_get_pwm_ptr();

		void set_microseconds( double microseconds);
		
	private:
		//void set_microseconds(uint16_t microseconds);
		double create_percent( double microseconds);


		//Updates percentage of duty cycle
		//over the range of possible duty cycle values
		void update_percent( double new_us );

		//Data
		enum BlackLib::pwmName pwm_pin;
		double initial_us;
		double arm_us;
		double low_us;
		double high_us;
		double current_us;
		double percent;
		BlackLib::BlackPWM  pwm;
		bool is_armed;

};

#endif
