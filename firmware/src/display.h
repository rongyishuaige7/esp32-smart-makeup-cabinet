#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include "sensors.h"

void initDisplay();
void displaySensors(SensorData data);
void displayMessage(const char* line1, const char* line2 = "");
void displayUVWarning(int uvValue);
void displayExpireWarning(const char* productName);
void displayWelcome();
void clearDisplay();
void updateDisplay();

#endif
