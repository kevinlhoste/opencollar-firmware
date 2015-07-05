#include <MS5611.h>

MS5611 ms5611;

uint32_t pressure = 0;

void
altimeter_setup(void)
{
	if(!ms5611.begin(MS5611_ULTRA_HIGH_RES)) {
		Serial.println("MS5611 failed!");
	} else {
		Serial.println("MS5611 OK");
	}
}

void
altimeter_get(void)
{
	pressure = ms5611.readPressure();
}

