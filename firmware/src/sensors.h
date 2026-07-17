#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

struct SensorData {
    float temperature;
    float humidity;
    int uvValue;
    bool pirDetected;
    bool climateAvailable;
    bool uvAvailable;
};

void initSensors();
SensorData readSensors();
void handlePIR();
void setPirLightTimeout(int seconds);

#endif
