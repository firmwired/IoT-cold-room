#include "ACS712.h"
#include "sensors.h"


float sensor_current(int pin, float volts, uint16_t max_ADC, float mVperAmp) {
  // Create an ACS712 object for the specified pin
  ACS712 sensor(pin, volts, max_ADC, mVperAmp);
  
  // Initialize the sensor
  sensor.autoMidPoint();
  
  // Read and return current in milliamps
  float currentmA = sensor.mA_AC_sampling();
  return currentmA;
}