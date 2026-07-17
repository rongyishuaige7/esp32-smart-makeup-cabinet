#include "sensors.h"
#include "config.h"
#include "actuators.h"
#include <DHT.h>
#include <Arduino.h>

DHT dht(PIN_DHT11, DHT11);
static SensorData currentData = {NAN, NAN, -1, false, false, false};
static unsigned long lastPirDetectedTime = 0;
static bool lightOnByPir = false;
static int pirLightTimeout = LIGHT_TIMEOUT;

void initSensors() {
    dht.begin();
    analogReadResolution(12);
    pinMode(PIN_UV_SENSOR, INPUT);
    analogSetPinAttenuation(PIN_UV_SENSOR, ADC_11db);
    pinMode(PIN_PIR, INPUT);
    Serial.println("Sensors initialized; availability is determined at each read");
}

SensorData readSensors() {
    const float hum = dht.readHumidity();
    const float temp = dht.readTemperature();
    currentData.climateAvailable = !isnan(hum) && !isnan(temp);
    if (currentData.climateAvailable) {
        currentData.humidity = hum;
        currentData.temperature = temp;
    } else {
        currentData.humidity = NAN;
        currentData.temperature = NAN;
        Serial.println("DHT read unavailable; climate automation will not run");
    }

    // This is an uncalibrated module-output estimate, not a UV safety, dose, or
    // health assessment. ADC readings are still reported only as an estimate.
    long uvSum = 0;
    for (int i = 0; i < 1024; ++i) {
        uvSum += analogRead(PIN_UV_SENSOR);
        delayMicroseconds(100);
    }
    const int uvAvg = uvSum >> 10;
    const float uvMillivolts = uvAvg * 3300.0f / 4095.0f;
    if      (uvMillivolts < 50)   currentData.uvValue = 0;
    else if (uvMillivolts < 227)  currentData.uvValue = 1;
    else if (uvMillivolts < 318)  currentData.uvValue = 2;
    else if (uvMillivolts < 408)  currentData.uvValue = 3;
    else if (uvMillivolts < 503)  currentData.uvValue = 4;
    else if (uvMillivolts < 606)  currentData.uvValue = 5;
    else if (uvMillivolts < 696)  currentData.uvValue = 6;
    else if (uvMillivolts < 795)  currentData.uvValue = 7;
    else if (uvMillivolts < 881)  currentData.uvValue = 8;
    else if (uvMillivolts < 976)  currentData.uvValue = 9;
    else if (uvMillivolts < 1079) currentData.uvValue = 10;
    else                           currentData.uvValue = 11;
    currentData.uvAvailable = true;
    currentData.pirDetected = digitalRead(PIN_PIR) == HIGH;
    return currentData;
}

void handlePIR() {
#if EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED
    const bool pirDetected = digitalRead(PIN_PIR) == HIGH;
    if (pirDetected) {
        lastPirDetectedTime = millis();
        if (!lightOnByPir) {
            lightOnByPir = true;
            setLEDPower(true);
        }
    } else if (lightOnByPir && millis() - lastPirDetectedTime > static_cast<unsigned long>(pirLightTimeout) * 1000UL) {
        lightOnByPir = false;
        setLEDPower(false);
    }
#else
    // Do not turn LED hardware on/off from PIR without both physical-output opt-ins.
#endif
}

void setPirLightTimeout(int seconds) {
    if (seconds > 0) pirLightTimeout = seconds;
}
