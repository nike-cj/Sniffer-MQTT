//______________________________________________________________________________
// libraries
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- class declaration ------------------------------------------------------
#include "Led.h"


//----- namespaces -------------------------------------------------------------
using namespace std;



//______________________________________________________________________________
// definition
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- static singleton instance ----------------------------------------------
LedClass Led;



//______________________________________________________________________________
// costructors and destructors
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- parametrized constructor -----------------------------------------------
LedClass::LedClass(int led_pin, float blinking_freq) {
	// store parameters
	_pin 		= led_pin;
	_frequency 	= blinking_freq;

	// initialize the GPIO
	pinMode(led_pin, OUTPUT);
}



//______________________________________________________________________________
// methods
//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

//----- activate ---------------------------------------------------------------
void LedClass::on(int milliseconds) {
	// set to 1 the GPIO signal
	digitalWrite(_pin, HIGH);

	// if timeout is set
	if (milliseconds > 0) {
		delay(milliseconds);
		off();
	}
}


//----- deactivate -------------------------------------------------------------
void LedClass::off() {
	// set to 0 the GPIO signal
	digitalWrite(_pin, LOW);
}


//----- turn on/off repeatly ---------------------------------------------------
bool LedClass::blink(int milliseconds) {
	// local variable
	int tot_time = 0;

	// convert Hz to milliseconds
	float period_ms = 1000 / _frequency;

	// blink at a given frequency, for the requested time
	while (tot_time < milliseconds) {
		on();
		delay(period_ms);
		off();
		delay(period_ms);

		tot_time += 2*period_ms;
	}
}
