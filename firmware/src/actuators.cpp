#include "actuators.h"
#include "config.h"
#include <Arduino.h>

#define NUM_LEDS 16
CRGB leds[NUM_LEDS];
static Servo lockServo;

// These are command-state variables, not physical feedback. There is no door
// position sensor, current measurement, or driver acknowledgement in this design.
static bool fanState = false;
static bool humidifierState = false;
static bool lockState = true;
static LEDState ledState = {false, 0, CRGB::Black};
static bool fanManual = false;
static bool humidifierManual = false;
static bool ledManual = false;
static unsigned long fanManualAt = 0;
static unsigned long humidifierManualAt = 0;
static unsigned long ledManualAt = 0;

static bool actuatorsEnabled() {
    return EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED == 1;
}

void initActuators() {
    // Builds without both physical-output opt-ins deliberately leave all unverified actuator pins as
    // inputs. Unknown driver polarity, pull-ups and external power make even
    // an attempted LOW output unsafe to describe as a physical OFF state.
#if EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED
    digitalWrite(PIN_FAN_INA, LOW);
    digitalWrite(PIN_FAN_INB, LOW);
    pinMode(PIN_FAN_INA, OUTPUT);
    pinMode(PIN_FAN_INB, OUTPUT);
    digitalWrite(PIN_FAN_INA, LOW);
    digitalWrite(PIN_FAN_INB, LOW);

    digitalWrite(PIN_HUMIDIFIER, LOW);
    pinMode(PIN_HUMIDIFIER, OUTPUT);
    digitalWrite(PIN_HUMIDIFIER, LOW);

    digitalWrite(PIN_LOCK_SERVO, LOW);
    pinMode(PIN_LOCK_SERVO, OUTPUT);
    digitalWrite(PIN_LOCK_SERVO, LOW);
    FastLED.addLeds<WS2812B, PIN_LED_STRIP, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(0);
    for (int i = 0; i < NUM_LEDS; ++i) leds[i] = CRGB::Black;
    FastLED.show();
    Serial.println("Experimental actuator pins initialized for a supervised low-voltage bench test");
#else
    pinMode(PIN_FAN_INA, INPUT);
    pinMode(PIN_FAN_INB, INPUT);
    pinMode(PIN_HUMIDIFIER, INPUT);
    pinMode(PIN_LOCK_SERVO, INPUT);
    pinMode(PIN_LED_STRIP, INPUT);
    Serial.println("Physical outputs disabled: unverified actuator pins are high impedance; no output driver is initialized");
#endif
}

static void applyLEDOutput() {
#if EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED
    if (ledState.power) {
        FastLED.setBrightness(ledState.brightness);
        for (int i = 0; i < NUM_LEDS; ++i) leds[i] = ledState.color;
    } else {
        FastLED.setBrightness(0);
        for (int i = 0; i < NUM_LEDS; ++i) leds[i] = CRGB::Black;
    }
    FastLED.show();
#else
    // No FastLED output is initialized or transmitted without both physical-output opt-ins.
#endif
}

void setLED(LEDState state, bool manual) {
    if (manual) {
        ledManual = true;
        ledManualAt = millis();
    }
    ledState = state;
    applyLEDOutput();
}

void setLEDPower(bool on) {
    if (ledManual) return;
    ledState.power = on;
    applyLEDOutput();
}

void setLEDbrightness(uint8_t brightness) {
    ledState.brightness = brightness;
    applyLEDOutput();
}

static void writeFanOutput(bool on) {
#if EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED
    digitalWrite(PIN_FAN_INA, on ? HIGH : LOW);
    digitalWrite(PIN_FAN_INB, LOW);
#else
    (void)on;
    // Builds without both physical-output opt-ins deliberately do not drive unverified actuator pins.
#endif
}

static void writeHumidifierOutput(bool on) {
#if EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED
    digitalWrite(PIN_HUMIDIFIER, on ? HIGH : LOW);
#else
    (void)on;
    // Builds without both physical-output opt-ins deliberately do not drive unverified actuator pins.
#endif
}

static void requestFan(bool on) {
    fanState = on;
    writeFanOutput(on);
    Serial.printf("Fan command: %s%s\n", on ? "ON" : "OFF", actuatorsEnabled() ? "" : " (physical outputs disabled)");
}

static void requestHumidifier(bool on) {
    humidifierState = on;
    writeHumidifierOutput(on);
    Serial.printf("Humidifier command: %s%s\n", on ? "ON" : "OFF", actuatorsEnabled() ? "" : " (physical outputs disabled)");
}

void setFan(bool on, bool manual) {
    fanManual = manual;
    fanManualAt = manual ? millis() : 0;
    requestFan(on);
}

void setHumidifier(bool on, bool manual) {
    humidifierManual = manual;
    humidifierManualAt = manual ? millis() : 0;
    requestHumidifier(on);
}

void autoControlFan(bool on) {
    if (!fanManual) requestFan(on);
}

void autoControlHumidifier(bool on) {
    if (!humidifierManual) requestHumidifier(on);
}

void tickManualOverride() {
    const unsigned long now = millis();
    const unsigned long timeoutMs = static_cast<unsigned long>(MANUAL_OVERRIDE_TIMEOUT) * 1000UL;
    if (fanManual && now - fanManualAt >= timeoutMs) {
        fanManual = false;
        fanManualAt = 0;
        Serial.println("Fan manual override expired");
    }
    if (humidifierManual && now - humidifierManualAt >= timeoutMs) {
        humidifierManual = false;
        humidifierManualAt = 0;
        Serial.println("Humidifier manual override expired");
    }
    if (ledManual && now - ledManualAt >= timeoutMs) {
        ledManual = false;
        ledManualAt = 0;
        Serial.println("LED manual override expired");
    }
}

static void servoWriteAndDetach(int angle) {
#if EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED
    lockServo.attach(PIN_LOCK_SERVO, 500, 2500);
    lockServo.write(angle);
    delay(1200);
    lockServo.write(angle);
    delay(50);
    lockServo.detach();
    Serial.printf("Servo command -> %d deg, detached\n", angle);
#else
    (void)angle;
    Serial.println("Servo command suppressed without both physical-output opt-ins");
#endif
}

void lockInit() {
    // Builds without both physical-output opt-ins never reposition an attached servo on boot.
#if EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED
    servoWriteAndDetach(SERVO_LOCKED_ANGLE);
#else
    Serial.println("Servo startup command suppressed without both physical-output opt-ins");
#endif
}

void setLock(bool locked) {
    lockState = locked;
    servoWriteAndDetach(locked ? SERVO_LOCKED_ANGLE : SERVO_UNLOCKED_ANGLE);
    Serial.printf("Latch command: %s%s\n", locked ? "CLOSE" : "OPEN", actuatorsEnabled() ? "" : " (physical outputs disabled)");
}

bool getFanState() { return fanState; }
bool getHumidifierState() { return humidifierState; }
bool getFanManual() { return fanManual; }
bool getHumidifierManual() { return humidifierManual; }
bool getLEDManual() { return ledManual; }
bool getLockState() { return lockState; }
LEDState getLEDState() { return ledState; }
