#include "config.h"
#include "sensors.h"
#include "actuators.h"
#include "display.h"
#include "http_server.h"
#include "storage.h"
#include "rfid.h"
#include <WiFi.h>

void applyRuntimeSettings(SystemSettings settings);
static SystemSettings runtimeSettings;
static unsigned long lastSensorRead = 0;
static const unsigned long SENSOR_READ_INTERVAL = 2000;
static unsigned long lastRFIDCheck = 0;
static const unsigned long RFID_CHECK_INTERVAL = 500;

void setup() {
    Serial.begin(115200);
#if EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED
    Serial.println("Smart Makeup Cabinet starting with local experimental physical outputs enabled...");
#else
    Serial.println("Smart Makeup Cabinet starting with physical outputs disabled...");
#endif
    initStorage();
    initActuators();
    initSensors();
    applyRuntimeSettings(loadSettings());
    initDisplay();
    initRFID();
    // Without both physical-output opt-ins this does not attach or move the servo.
    lockInit();
    initHTTPServer();
    displayWelcome();
#if EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED
    Serial.println("Initialized: local experimental physical outputs and optional local HTTP may be enabled only with non-empty local Wi-Fi credentials.");
#else
    Serial.println("Initialized: physical outputs and Wi-Fi/HTTP are disabled without both physical-output opt-ins.");
#endif
}

void applyRuntimeSettings(SystemSettings settings) {
    runtimeSettings = settings;
    setPirLightTimeout(settings.lightTimeout);
}

void loop() {
    const unsigned long now = millis();
    if (now - lastSensorRead > SENSOR_READ_INTERVAL) {
        lastSensorRead = now;
        const SensorData data = readSensors();
        displaySensors(data);
        if (data.uvAvailable && data.uvValue > 6) displayUVWarning(data.uvValue);

#if EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED
        // Only a build with both physical-output opt-ins may change outputs from a fresh,
        // valid DHT reading. An unavailable reading never drives automation.
        if (data.climateAvailable) {
            if (data.temperature > runtimeSettings.tempThresholdHigh) autoControlFan(true);
            else if (data.temperature < runtimeSettings.tempThresholdLow) autoControlFan(false);
            if (data.humidity < runtimeSettings.humidThresholdLow) autoControlHumidifier(true);
            else if (data.humidity > runtimeSettings.humidThresholdHigh) autoControlHumidifier(false);
        }
#endif
    }

#if EXPERIMENTAL_RFID_MAINTENANCE_ENABLED
    if (now - lastRFIDCheck > RFID_CHECK_INTERVAL) {
        lastRFIDCheck = now;
        // This maintenance-only path may notice a card for a private local
        // workflow, but it deliberately never treats a cloneable UID as an
        // identity credential and never commands the latch.
        if (isRFIDScanMode()) handleRFIDScan();
    }
#endif

    tickManualOverride();
    handlePIR();
    handleHTTPServer();
    delay(50);
}
