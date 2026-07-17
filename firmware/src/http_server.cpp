#include "http_server.h"
#include "config.h"
#include "sensors.h"
#include "actuators.h"
#include "storage.h"
#include "rfid.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <stdlib.h>
#include <time.h>

extern void applyRuntimeSettings(SystemSettings settings);
static WebServer server(80);
static bool httpStarted = false;

static void sendJson(int code, const String& content) {
    server.send(code, "application/json; charset=utf-8", content);
}

static void sendError(int code, const char* error) {
    DynamicJsonDocument document(96);
    document["accepted"] = false;
    document["error"] = error;
    String response;
    serializeJson(document, response);
    sendJson(code, response);
}

static bool parseBody(DynamicJsonDocument& document) {
    if (!server.hasHeader("Content-Type") || !server.header("Content-Type").startsWith("application/json") ||
        !server.hasArg("plain")) return false;
    const String body = server.arg("plain");
    if (body.length() == 0 || body.length() > MAX_REQUEST_BODY_BYTES) return false;
    return !deserializeJson(document, body);
}

static bool hasOnlyFields(JsonObjectConst object, std::initializer_list<const char*> allowed) {
    for (JsonPairConst item : object) {
        bool known = false;
        for (const char* field : allowed) {
            if (item.key() == field) { known = true; break; }
        }
        if (!known) return false;
    }
    return true;
}

static bool getStrictBool(const JsonVariantConst& value, bool& output) {
    if (!value.is<bool>()) return false;
    output = value.as<bool>();
    return true;
}

static bool getBoundedInt(const JsonVariantConst& value, int minValue, int maxValue, int& output) {
    if (!value.is<int>()) return false;
    const int candidate = value.as<int>();
    if (candidate < minValue || candidate > maxValue) return false;
    output = candidate;
    return true;
}

static bool validUid(const String& uid) {
    if (uid.length() < 2 || uid.length() > MAX_RFID_UID_BYTES) return false;
    for (size_t i = 0; i < uid.length(); ++i) {
        const char c = uid[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F') || c == ':')) return false;
    }
    return true;
}

static bool validTextField(const String& text, size_t maximum, bool allowEmpty = false) {
    if ((text.length() == 0 && !allowEmpty) || text.length() > maximum) return false;
    for (size_t i = 0; i < text.length(); ++i) {
        if (static_cast<unsigned char>(text[i]) < 0x20) return false;
    }
    return true;
}

static bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static bool validDate(const String& value) {
    if (value.length() == 0) return true;
    if (value.length() != 10 || value[4] != '-' || value[7] != '-') return false;
    for (size_t i = 0; i < value.length(); ++i) {
        if (i == 4 || i == 7) continue;
        if (value[i] < '0' || value[i] > '9') return false;
    }
    const int year = value.substring(0, 4).toInt();
    const int month = value.substring(5, 7).toInt();
    const int day = value.substring(8, 10).toInt();
    if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1) return false;
    static const int monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maximum = monthDays[month - 1];
    if (month == 2 && isLeapYear(year)) maximum = 29;
    return day <= maximum;
}

static bool controlEnabled() {
    return EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED == 1;
}

static const char* expiryText(ExpiryState state) {
    switch (state) {
        case ExpiryState::NotSet: return "not_set";
        case ExpiryState::Valid: return "valid";
        case ExpiryState::Expired: return "expired";
        case ExpiryState::TimeUnavailable: return "time_unavailable";
        default: return "invalid_record";
    }
}

static void handleStatus() {
    DynamicJsonDocument document(384);
    document["publicDefaultInert"] = !controlEnabled();
    document["localControlAndActuatorsEnabled"] = controlEnabled();
    document["ledCommandedOn"] = getLEDState().power;
    document["fanCommandedOn"] = getFanState();
    document["fanManual"] = getFanManual();
    document["humidifierCommandedOn"] = getHumidifierState();
    document["humidifierManual"] = getHumidifierManual();
    document["latchCommandedClosed"] = getLockState();
    document["physicalPositionVerified"] = false;
    String response;
    serializeJson(document, response);
    sendJson(200, response);
}

static void handleSensors() {
    const SensorData data = readSensors();
    DynamicJsonDocument document(256);
    document["climate"] = data.climateAvailable ? "available" : "unavailable";
    if (data.climateAvailable) {
        document["temp"] = data.temperature;
        document["humid"] = data.humidity;
    } else {
        document["temp"] = nullptr;
        document["humid"] = nullptr;
    }
    document["uvEstimate"] = data.uvAvailable;
    if (data.uvAvailable) document["uv"] = data.uvValue;
    else document["uv"] = nullptr;
    document["pir"] = data.pirDetected;
    String response;
    serializeJson(document, response);
    sendJson(200, response);
}

static bool requireControl() {
    if (controlEnabled()) return true;
    sendError(403, "experimental_local_control_disabled");
    return false;
}

static void handleLED() {
    if (!requireControl()) return;
    DynamicJsonDocument document(384);
    if (!parseBody(document) || !document.is<JsonObject>() || !hasOnlyFields(document.as<JsonObjectConst>(), {"power", "brightness", "color"})) { sendError(400, "invalid_body"); return; }
    LEDState state = getLEDState();
    if (document.size() == 0) { sendError(400, "missing_fields"); return; }
    if (!document["power"].isNull()) {
        bool value;
        if (!getStrictBool(document["power"], value)) { sendError(400, "invalid_power"); return; }
        state.power = value;
    }
    if (!document["brightness"].isNull()) {
        int value;
        if (!getBoundedInt(document["brightness"], 0, 255, value)) { sendError(400, "invalid_brightness"); return; }
        state.brightness = static_cast<uint8_t>(value);
    }
    if (!document["color"].isNull()) {
        JsonArray color = document["color"].as<JsonArray>();
        int red, green, blue;
        if (color.isNull() || color.size() != 3 || !getBoundedInt(color[0], 0, 255, red) ||
            !getBoundedInt(color[1], 0, 255, green) || !getBoundedInt(color[2], 0, 255, blue)) { sendError(400, "invalid_color"); return; }
        state.color = CRGB(red, green, blue);
    }
    setLED(state, true);
    sendJson(200, "{\"accepted\":true}");
}

static void handleFan() {
    if (!requireControl()) return;
    DynamicJsonDocument document(128);
    if (!parseBody(document) || !document.is<JsonObject>() || !hasOnlyFields(document.as<JsonObjectConst>(), {"power", "manual"})) { sendError(400, "invalid_body"); return; }
    bool power, manual = true;
    if (!getStrictBool(document["power"], power) || (!document["manual"].isNull() && !getStrictBool(document["manual"], manual))) { sendError(400, "invalid_fan_request"); return; }
    setFan(power, manual);
    sendJson(200, "{\"accepted\":true}");
}

static void handleHumidifier() {
    if (!requireControl()) return;
    DynamicJsonDocument document(128);
    if (!parseBody(document) || !document.is<JsonObject>() || !hasOnlyFields(document.as<JsonObjectConst>(), {"power", "manual"})) { sendError(400, "invalid_body"); return; }
    bool power, manual = true;
    if (!getStrictBool(document["power"], power) || (!document["manual"].isNull() && !getStrictBool(document["manual"], manual))) { sendError(400, "invalid_humidifier_request"); return; }
    setHumidifier(power, manual);
    sendJson(200, "{\"accepted\":true}");
}

static void handleLatch() {
    if (!requireControl()) return;
    DynamicJsonDocument document(128);
    if (!parseBody(document) || !document.is<JsonObject>() || !hasOnlyFields(document.as<JsonObjectConst>(), {"action"})) { sendError(400, "invalid_body"); return; }
    if (!document["action"].is<const char*>()) { sendError(400, "invalid_latch_action"); return; }
    const String action = document["action"].as<String>();
    if (action == "open") setLock(false);
    else if (action == "close") setLock(true);
    else { sendError(400, "invalid_latch_action"); return; }
    sendJson(200, "{\"accepted\":true,\"physicalPositionVerified\":false}");
}

static void handleRFIDRegister() {
#if EXPERIMENTAL_RFID_MAINTENANCE_ENABLED && EXPERIMENTAL_PHYSICAL_OUTPUTS_ENABLED
    DynamicJsonDocument document(512);
    if (!parseBody(document) || !document.is<JsonObject>() || !hasOnlyFields(document.as<JsonObjectConst>(), {"uid", "name", "brand", "expireDate"})) { sendError(400, "invalid_body"); return; }
    if (!document["uid"].is<const char*>() || !document["name"].is<const char*>() ||
        (!document["brand"].isNull() && !document["brand"].is<const char*>()) ||
        (!document["expireDate"].isNull() && !document["expireDate"].is<const char*>())) { sendError(400, "invalid_cosmetic_record"); return; }
    const String uid = document["uid"].as<String>();
    const String name = document["name"].as<String>();
    const String brand = document["brand"].isNull() ? "" : document["brand"].as<String>();
    const String expireDate = document["expireDate"].isNull() ? "" : document["expireDate"].as<String>();
    if (!validUid(uid) || !validTextField(name, MAX_COSMETIC_NAME_BYTES) ||
        !validTextField(brand, MAX_COSMETIC_BRAND_BYTES, true) || !validDate(expireDate)) { sendError(400, "invalid_cosmetic_record"); return; }
    if (!registerCosmetic(uid, name, brand, expireDate)) { sendError(409, "record_not_saved_or_duplicate"); return; }
    sendJson(200, "{\"accepted\":true}");
#else
    sendError(403, "experimental_rfid_maintenance_disabled");
#endif
}

static void handleRFIDList() {
    const int count = getCosmeticCount();
    DynamicJsonDocument document(8192);
    JsonArray cosmetics = document.to<JsonArray>();
    for (int i = 0; i < count; ++i) {
        const Cosmetic cosmetic = loadCosmetic(i);
        if (cosmetic.uid.length() == 0) continue;
        JsonObject item = cosmetics.createNestedObject();
        item["name"] = cosmetic.name;
        item["brand"] = cosmetic.brand;
        item["expireDate"] = cosmetic.expireDate;
        item["expiry"] = expiryText(getExpiryState(cosmetic.uid));
    }
    String response;
    serializeJson(document, response);
    sendJson(200, response);
}

static void handleSettingsGet() {
    const SystemSettings settings = loadSettings();
    DynamicJsonDocument document(256);
    document["tempThresholdHigh"] = settings.tempThresholdHigh;
    document["tempThresholdLow"] = settings.tempThresholdLow;
    document["humidThresholdHigh"] = settings.humidThresholdHigh;
    document["humidThresholdLow"] = settings.humidThresholdLow;
    document["lightTimeout"] = settings.lightTimeout;
    String response;
    serializeJson(document, response);
    sendJson(200, response);
}

static void handleSettingsPost() {
    if (!requireControl()) return;
    DynamicJsonDocument document(256);
    if (!parseBody(document) || !document.is<JsonObject>() || !hasOnlyFields(document.as<JsonObjectConst>(), {"tempThresholdHigh", "tempThresholdLow", "humidThresholdHigh", "humidThresholdLow", "lightTimeout"})) { sendError(400, "invalid_body"); return; }
    if (document.size() == 0) { sendError(400, "missing_fields"); return; }
    SystemSettings settings = loadSettings();
    int value;
    if (!document["tempThresholdHigh"].isNull()) { if (!getBoundedInt(document["tempThresholdHigh"], 10, 45, value)) { sendError(400, "invalid_temperature_threshold"); return; } settings.tempThresholdHigh = value; }
    if (!document["tempThresholdLow"].isNull()) { if (!getBoundedInt(document["tempThresholdLow"], 5, 40, value)) { sendError(400, "invalid_temperature_threshold"); return; } settings.tempThresholdLow = value; }
    if (!document["humidThresholdHigh"].isNull()) { if (!getBoundedInt(document["humidThresholdHigh"], 10, 95, value)) { sendError(400, "invalid_humidity_threshold"); return; } settings.humidThresholdHigh = value; }
    if (!document["humidThresholdLow"].isNull()) { if (!getBoundedInt(document["humidThresholdLow"], 5, 90, value)) { sendError(400, "invalid_humidity_threshold"); return; } settings.humidThresholdLow = value; }
    if (!document["lightTimeout"].isNull()) { if (!getBoundedInt(document["lightTimeout"], 1, 3600, value)) { sendError(400, "invalid_light_timeout"); return; } settings.lightTimeout = value; }
    if (settings.tempThresholdLow >= settings.tempThresholdHigh || settings.humidThresholdLow >= settings.humidThresholdHigh) { sendError(400, "invalid_threshold_order"); return; }
    saveSettings(settings);
    applyRuntimeSettings(settings);
    sendJson(200, "{\"accepted\":true}");
}

static void handleRFIDScanStart() { sendError(403, "raw_rfid_scan_not_available_over_public_http"); }
static void handleRFIDScanStop() { sendError(403, "raw_rfid_scan_not_available_over_public_http"); }
static void handleRFIDScanResult() { sendError(403, "raw_rfid_scan_not_available_over_public_http"); }
static void handleRFIDDelete() { sendError(403, "raw_rfid_maintenance_not_available_over_public_http"); }

static void handleRoot() {
    server.send(200, "text/plain; charset=utf-8",
                "Smart Makeup Cabinet local prototype API. This does not prove hardware is online or an actuator moved.");
}

static void configureRoutes() {
    const char* headers[] = {"Content-Type"};
    server.collectHeaders(headers, 1);
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/sensors", HTTP_GET, handleSensors);
    server.on("/api/led", HTTP_POST, handleLED);
    server.on("/api/fan", HTTP_POST, handleFan);
    server.on("/api/humidifier", HTTP_POST, handleHumidifier);
    server.on("/api/latch", HTTP_POST, handleLatch);
    server.on("/api/rfid/register", HTTP_POST, handleRFIDRegister);
    server.on("/api/rfid/list", HTTP_GET, handleRFIDList);
    server.on("/api/rfid/delete", HTTP_POST, handleRFIDDelete);
    server.on("/api/rfid/scan/start", HTTP_POST, handleRFIDScanStart);
    server.on("/api/rfid/scan/stop", HTTP_POST, handleRFIDScanStop);
    server.on("/api/rfid/scan/result", HTTP_GET, handleRFIDScanResult);
    server.on("/api/settings", HTTP_GET, handleSettingsGet);
    server.on("/api/settings", HTTP_POST, handleSettingsPost);
    server.onNotFound([]() { sendError(404, "not_found"); });
}

void initHTTPServer() {
    configureRoutes();
    if (WIFI_SSID[0] == '\0' || !controlEnabled()) {
        WiFi.mode(WIFI_OFF);
        Serial.println("[WiFi] public default keeps Wi-Fi and HTTP disabled");
        return;
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    const unsigned long startedAt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 15000UL) delay(100);
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect(false, false);
        WiFi.mode(WIFI_OFF);
        Serial.println("[WiFi] connection unavailable; HTTP remains disabled");
        return;
    }
    setenv("TZ", "CST-8", 1);
    tzset();
    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    server.begin();
    httpStarted = true;
    Serial.println("[WiFi] local experimental HTTP service started");
}

void handleHTTPServer() {
    if (httpStarted) server.handleClient();
}
