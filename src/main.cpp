#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define DHTPIN 12
#define DHTTYPE DHT22
#define BluePin 26
#define GreenPin 13
#define RedPin 14
#define POT_LEAK 34

#define STEP_PIN 32
#define DIR_PIN 33

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* nodeRedEndpoint = "http://4.tcp.eu.ngrok.io:10994/api/telemetry";

// condition de chambre froide
const float target_temp = 4.5;  // middle point (+2°C et +8°C)
const float criticalHigh = 8.0;  // maximum temperature
const float criticalLow = 2.0; // minimum temp
const int leakTreshold = 20;    // seuil de fuites (simulation)

const int valveMax_steps = 200;
int valvePositionSteps = 0;
bool coolingActive = false;

float integral_error = 0.0;

// time scale multplier | 
// 60.0 = 1min of real life passes every 1 second, 1hr of real life = 1min of simulation time
// 360.0 = 6 minutes/1s 
// 120.0 = 2 minutes/1s
const float SIMULATION_SPEED = 360.0;

// datasheet parameters du vannes motorisée
const float V_supply_voltage = 24.0;
const float logicPower_mw = 264.0;
const float I_stepping_ma = 1200.0;
const float I_holding_ma = 400.0;

// simulation du engine physique
float internalColdRoomTemp = 10.0; // temp INITIAL
unsigned long lastPhysicsTime = 1000;

unsigned long lastLoopTime = 0;
const unsigned long Interval = 1000; // 1s

float totalEnergy_mWh = 0.0;
unsigned long lastEnergyCalc = 0;
bool motorMovedTHisCylce = false;


float simulatedAmbient = 25.0;
unsigned long lastDriftTime = 0;
const unsigned long driftInterval = 5000;



void setColor(int red, int green, int blue) {
    analogWrite(RedPin, red);
    analogWrite(BluePin, blue);
    analogWrite(GreenPin, green);
}

void setValvePostion(int targetSteps) {
    targetSteps = constrain(targetSteps, 0, valveMax_steps);
    int stepDiff = targetSteps - valvePositionSteps;

    if (stepDiff != 0) {
        motorMovedTHisCylce = true;
        digitalWrite(DIR_PIN, stepDiff > 0 ? HIGH : LOW);
        int stepsToMove = abs(stepDiff);
        for (int i = 0; i < stepsToMove; i++) {
            digitalWrite(STEP_PIN, HIGH);
            delayMicroseconds(800);
            digitalWrite(STEP_PIN, LOW);
            delayMicroseconds(800);
        }
        valvePositionSteps = targetSteps;
    } else {
        motorMovedTHisCylce = false;
    } 
    coolingActive = (valvePositionSteps > 0);
}

void updateThermalPhysics(float ambientTemp) {
    unsigned long now = millis();
    if (lastPhysicsTime == 0) {
        lastPhysicsTime = now;
        return;
    }

    //  simulation speed multiplier
    float dt_real = (now - lastPhysicsTime) / 1000.0;
    lastPhysicsTime = now;

  
    float dt = dt_real * SIMULATION_SPEED;


    // difference thermique. it takes 4hours for
    float insulationLossRate = 0.000015;
    // industrial rooms drop 4°C over 2Hrs 4/72000s
    float maxCoolingRate = 0.00055; // °C drop per sec at 100% duty

    float heatGain = (ambientTemp - internalColdRoomTemp) * insulationLossRate;
    float valvePct = (float)valvePositionSteps / valveMax_steps;
    
    float heatExtracted = valvePct * maxCoolingRate;

    internalColdRoomTemp += (heatGain - heatExtracted) * dt;
    internalColdRoomTemp = constrain(internalColdRoomTemp, 0.5, ambientTemp);
    
    
}

void setup() {
    Serial.begin(115200);
    dht.begin();

    pinMode(RedPin, OUTPUT);
    pinMode(GreenPin, OUTPUT);
    pinMode(BluePin, OUTPUT);
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
   
    setColor(255, 255, 0); // BOOT
     
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
    }
    setColor(0, 255, 0);
    lastEnergyCalc = millis();
    Serial.println("timestamp_ms,room_temp,ambient_temp,humidity,leak_pct,leak_alert,cooling,valve_pct,power_mW,energy_mWh,sensor_fault");
}

void loop() {
    unsigned long now = millis();

    if (now - lastLoopTime < Interval) return;
    lastLoopTime = now;

    if (now - lastDriftTime >= driftInterval) {
        simulatedAmbient += 0.1;
        if (simulatedAmbient > 60.0) simulatedAmbient = 25.0;
        lastDriftTime = now;
    }

    float ambientTemp = dht.readTemperature();
    if (ambientTemp > 50.0) ambientTemp = 50.0;
    float hum = dht.readHumidity();
    bool sensorFault = isnan(ambientTemp) || isnan(hum);
    if (isnan(ambientTemp)) ambientTemp = 25.0; // thermal fallback

    int rawADC = analogRead(POT_LEAK);
    float rawMilliVolts = (rawADC / 4095.0) * 3300.0; // ESP32 12bit ADC @ 3.3V AREF
    int leakPercent  = map(rawADC, 0, 4095, 0, 100);
    bool isLeak = (leakPercent > leakTreshold);

    // Run the negine
    // updateThermalPhysics(ambientTemp);
    updateThermalPhysics(simulatedAmbient);
    float currentTemp = internalColdRoomTemp;

    if (sensorFault || currentTemp <= criticalLow) {
        setValvePostion(0); 
        integral_error = 0.0;
    } else {
        float error = currentTemp - target_temp;
        integral_error += error * 1.0; // accumulateur d'integral
        integral_error = constrain(integral_error, -100, 500);  // limits convergence vers l'infinie

        float kp = 15.0; 
        float ki = 0.5;  

        float p_action = error * kp;
        float i_action = integral_error * ki;

        // sommation des control action et conversion vers stepper motor movements
        int calculatedSteps = (int)(p_action + i_action);
        calculatedSteps = constrain(calculatedSteps, 0, valveMax_steps);
        setValvePostion(calculatedSteps);
        
    if (error < 0) {
            integral_error *= 0.95; 
        }
    }


    // puissance et courant (estimation proche)
    float active_current_mA = 0.0;
    if (motorMovedTHisCylce) {
        active_current_mA = I_stepping_ma; 
    } else if (coolingActive) {
        active_current_mA = I_holding_ma; 
    } else {
        active_current_mA = 0.0;
    }

    float instantPuissance_mW = logicPower_mw + (active_current_mA * V_supply_voltage);
    
    float hours = ((now - lastEnergyCalc) / 3600000.0) * SIMULATION_SPEED; 
    totalEnergy_mWh += instantPuissance_mW * hours;
    lastEnergyCalc = now;

    // alarm et led status
    if (sensorFault || isLeak ||currentTemp >=criticalHigh || currentTemp <=criticalLow) {
        setColor(255, 0, 0); 
    } else if (currentTemp > 6.5 || currentTemp < 3.0) {
        setColor(255, 165, 0);
    } else {
        setColor(0, 255, 0);
    }
     
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(nodeRedEndpoint);
        http.addHeader("Content-Type", "application/json");

        StaticJsonDocument<348> doc;
        doc["device_id"]         = "ESP32_EEV_CONTROLLER";
        doc["temperature"]       = round(currentTemp * 10.0) / 10.0;
        doc["ambient_temp"]      = simulatedAmbient;
        doc["humidity"]           = hum;
        doc["adc_raw"]           = rawADC;
        doc["adc_mv"]            = round(rawMilliVolts);
        doc["leak_level"]        = leakPercent;
        doc["leak_alert"]        = isLeak;
        doc["cooling"]           = coolingActive;
        doc["valve_position_pct"]= (valvePositionSteps * 100) / valveMax_steps;
        doc["motor_current_mA"]  = active_current_mA;
        doc["power_mW"]          = instantPuissance_mW;
        doc["energy_mWh"]        = totalEnergy_mWh;
        doc["sensor_fault"]      = sensorFault;
        doc["rssi_dBm"]          = WiFi.RSSI();

        String json;
        serializeJson(doc, json);
        http.POST(json);
        http.end();
        
    }

    Serial.print(now); Serial.print(",");
    Serial.print(currentTemp, 2); Serial.print(",");
    Serial.print(simulatedAmbient, 1); Serial.print(",");
    Serial.print(hum, 1); Serial.print(",");
    Serial.print(leakPercent); Serial.print(",");
    Serial.print(isLeak); Serial.print(",");
    Serial.print(coolingActive); Serial.print(",");
    Serial.print((valvePositionSteps * 100) / valveMax_steps); Serial.print(",");
    Serial.print(instantPuissance_mW, 1); Serial.print(",");
    Serial.print(totalEnergy_mWh, 2); Serial.print(",");
    Serial.println(sensorFault);
}




