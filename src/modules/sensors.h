#pragma once

#include <Arduino.h>  

// Calculate current from an ACS712 sensor on a specific pin
// IMPORTANT: When WiFi is enabled on ESP32, use only ADC1 pins:
//   ADC1 pins (safe with WiFi): GPIO32, GPIO33, GPIO34, GPIO35, GPIO36, GPIO37, GPIO38, GPIO39
//   ADC2 pins (conflict with WiFi): GPIO0, GPIO2, GPIO4, GPIO12, GPIO13, GPIO14, GPIO15, GPIO25, GPIO26, GPIO27
//
// pin: analog pin number where the sensor is connected (use ADC1 pins!)
// volts: reference voltage (usually 3.3V for ESP32 or 5.0V for Arduino)
// max_ADC: maximum ADC value (4095 for ESP32, 1023 for Arduino)
// mVperAmp: sensitivity (185 for 5A, 100 for 20A, 66 for 30A)
// returns: current in milliamps
float sensor_current(int pin, float volts, uint16_t max_ADC, float mVperAmp);
