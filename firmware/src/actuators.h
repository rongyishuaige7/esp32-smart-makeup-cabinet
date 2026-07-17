#ifndef ACTUATORS_H
#define ACTUATORS_H

#include <Arduino.h>
#include <FastLED.h>
#include <ESP32Servo.h>

struct LEDState {
    bool power;
    uint8_t brightness;
    CRGB color;
};

void initActuators();
void lockInit();   // Only moves a servo when both local physical-output opt-ins are exactly enabled.
void setLEDPower(bool on);
void setLEDbrightness(uint8_t brightness);

// Calls do not drive physical outputs unless both local physical-output opt-ins are exactly enabled. manual=true controls a local experimental override.
void setFan(bool on, bool manual = false);
void setHumidifier(bool on, bool manual = false);
void setLED(LEDState state, bool manual = false);
void setLock(bool locked);

// Automatic calls do not drive physical outputs unless both local physical-output opt-ins are exactly enabled.
void autoControlFan(bool on);
void autoControlHumidifier(bool on);

// 每次 loop 调用，检查手动覆盖是否超时并自动清除
void tickManualOverride();

// 获取当前执行器状态
bool getFanState();
bool getHumidifierState();
bool getFanManual();
bool getHumidifierManual();
bool getLEDManual();
bool getLockState();
LEDState getLEDState();

#endif
