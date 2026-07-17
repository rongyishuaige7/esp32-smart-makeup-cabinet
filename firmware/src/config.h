#ifndef SMART_MAKEUP_CABINET_CONFIG_H
#define SMART_MAKEUP_CABINET_CONFIG_H

// Public firmware deliberately has no embedded Wi-Fi credential. A local,
// ignored config.local.h can supply values only for a supervised test.
#if __has_include("config.local.h")
#include "config.local.h"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

// GPIO mapping derived from the original source. It is not a measured wiring diagram.
#define PIN_DHT11 4
#define PIN_UV_SENSOR 34
#define PIN_PIR 5
#define PIN_LED_STRIP 27
#define PIN_FAN_INA 26
#define PIN_FAN_INB 33
#define PIN_HUMIDIFIER 25
#define PIN_LOCK_SERVO 14
#define PIN_OLED_SDA 21
#define PIN_OLED_SCL 22
#define PIN_RC522_SCK 18
#define PIN_RC522_MISO 19
#define PIN_RC522_MOSI 23
#define PIN_RC522_SS 2
#define PIN_RC522_RST 15

#define SERVO_LOCKED_ANGLE 10
#define SERVO_UNLOCKED_ANGLE 170

#define TEMP_THRESHOLD_HIGH 28
#define TEMP_THRESHOLD_LOW 25
#define HUMID_THRESHOLD_LOW 75
#define HUMID_THRESHOLD_HIGH 85
#define LIGHT_TIMEOUT 10
#define MANUAL_OVERRIDE_TIMEOUT 10

// Public API input limits. They are not a network security boundary.
#define MAX_REQUEST_BODY_BYTES 768
#define MAX_COSMETIC_NAME_BYTES 48
#define MAX_COSMETIC_BRAND_BYTES 48
#define MAX_RFID_UID_BYTES 32
#define MAX_COSMETIC_RECORDS 50

// The public build is intentionally inert with respect to physical actuators.
// A local ignored config.local.h may explicitly opt in for a documented
// low-voltage bench test; that opt-in is not a security boundary.
#ifndef ENABLE_EXPERIMENTAL_ACTUATORS
#define ENABLE_EXPERIMENTAL_ACTUATORS 0
#endif

#ifndef ENABLE_EXPERIMENTAL_LOCAL_CONTROL
#define ENABLE_EXPERIMENTAL_LOCAL_CONTROL 0
#endif

#ifndef ENABLE_EXPERIMENTAL_RFID_MAINTENANCE
#define ENABLE_EXPERIMENTAL_RFID_MAINTENANCE 0
#endif

// Physical-output code is reachable only when both independent local opt-ins
// are present. Do not treat this compile-time predicate as electrical safety,
// authorization, network security, or proof of a physical result.
#if ENABLE_EXPERIMENTAL_ACTUATORS == 1 && ENABLE_EXPERIMENTAL_LOCAL_CONTROL == 1
#define EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED 1
#else
#define EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED 0
#endif

// RFID maintenance is intentionally independent from physical-output control,
// but still requires the exact opt-in value. It never authorizes or commands
// a latch and it never exposes raw UID over public HTTP.
#if ENABLE_EXPERIMENTAL_RFID_MAINTENANCE == 1
#define EXPERIMENTAL_RFID_MAINTENANCE_ENABLED 1
#else
#define EXPERIMENTAL_RFID_MAINTENANCE_ENABLED 0
#endif

#endif
