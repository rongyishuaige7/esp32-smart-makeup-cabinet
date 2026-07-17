#include "storage.h"
#include "config.h"
#include <Preferences.h>
#include <ArduinoJson.h>

static Preferences prefSettings;
static Preferences prefCosmetics;
static const char* NAMESPACE_SETTINGS = "settings";
static const char* NAMESPACE_COSMETICS = "cosmetics";
static const char* KEY_COSMETIC_COUNT = "count";
static const char* KEY_COSMETIC_PREFIX = "cosmetic_";

static const SystemSettings defaultSettings = {
    .tempThresholdHigh = TEMP_THRESHOLD_HIGH,
    .tempThresholdLow = TEMP_THRESHOLD_LOW,
    .humidThresholdHigh = HUMID_THRESHOLD_HIGH,
    .humidThresholdLow = HUMID_THRESHOLD_LOW,
    .lightTimeout = LIGHT_TIMEOUT
};

static int boundedCount(int count) {
    if (count < 0) return 0;
    if (count > MAX_COSMETIC_RECORDS) return MAX_COSMETIC_RECORDS;
    return count;
}

void initStorage() {
    prefSettings.begin(NAMESPACE_SETTINGS, false);
    if (!prefSettings.isKey("tmpHigh")) {
        prefSettings.putInt("tmpHigh", defaultSettings.tempThresholdHigh);
        prefSettings.putInt("tmpLow", defaultSettings.tempThresholdLow);
        prefSettings.putInt("humHigh", defaultSettings.humidThresholdHigh);
        prefSettings.putInt("humLow", defaultSettings.humidThresholdLow);
        prefSettings.putInt("lightTO", defaultSettings.lightTimeout);
    }
    prefSettings.end();
    prefCosmetics.begin(NAMESPACE_COSMETICS, false);
    if (!prefCosmetics.isKey(KEY_COSMETIC_COUNT)) prefCosmetics.putInt(KEY_COSMETIC_COUNT, 0);
    prefCosmetics.end();
}

void saveSettings(SystemSettings settings) {
    prefSettings.begin(NAMESPACE_SETTINGS, false);
    prefSettings.putInt("tmpHigh", settings.tempThresholdHigh);
    prefSettings.putInt("tmpLow", settings.tempThresholdLow);
    prefSettings.putInt("humHigh", settings.humidThresholdHigh);
    prefSettings.putInt("humLow", settings.humidThresholdLow);
    prefSettings.putInt("lightTO", settings.lightTimeout);
    prefSettings.end();
}

SystemSettings loadSettings() {
    SystemSettings settings;
    prefSettings.begin(NAMESPACE_SETTINGS, false);
    settings.tempThresholdHigh = prefSettings.getInt("tmpHigh", defaultSettings.tempThresholdHigh);
    settings.tempThresholdLow = prefSettings.getInt("tmpLow", defaultSettings.tempThresholdLow);
    settings.humidThresholdHigh = prefSettings.getInt("humHigh", defaultSettings.humidThresholdHigh);
    settings.humidThresholdLow = prefSettings.getInt("humLow", defaultSettings.humidThresholdLow);
    settings.lightTimeout = prefSettings.getInt("lightTO", defaultSettings.lightTimeout);
    prefSettings.end();
    if (settings.tempThresholdLow >= settings.tempThresholdHigh ||
        settings.humidThresholdLow >= settings.humidThresholdHigh ||
        settings.lightTimeout < 1 || settings.lightTimeout > 3600) {
        return defaultSettings;
    }
    return settings;
}

bool saveCosmetic(Cosmetic cosmetic) {
    prefCosmetics.begin(NAMESPACE_COSMETICS, false);
    const int count = boundedCount(prefCosmetics.getInt(KEY_COSMETIC_COUNT, 0));
    if (count >= MAX_COSMETIC_RECORDS) { prefCosmetics.end(); return false; }
    StaticJsonDocument<256> doc;
    doc["uid"] = cosmetic.uid;
    doc["name"] = cosmetic.name;
    doc["brand"] = cosmetic.brand;
    doc["expireDate"] = cosmetic.expireDate;
    doc["registeredAt"] = cosmetic.registeredAt;
    char jsonBuffer[256];
    const size_t written = serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
    if (written == 0 || written >= sizeof(jsonBuffer)) { prefCosmetics.end(); return false; }
    char key[32];
    snprintf(key, sizeof(key), "%s%d", KEY_COSMETIC_PREFIX, count);
    if (prefCosmetics.putString(key, jsonBuffer) == 0) { prefCosmetics.end(); return false; }
    if (prefCosmetics.putInt(KEY_COSMETIC_COUNT, count + 1) == 0) { prefCosmetics.remove(key); prefCosmetics.end(); return false; }
    prefCosmetics.end();
    return true;
}

Cosmetic loadCosmetic(int index) {
    Cosmetic cosmetic{"", "", "", "", 0};
    prefCosmetics.begin(NAMESPACE_COSMETICS, false);
    const int count = boundedCount(prefCosmetics.getInt(KEY_COSMETIC_COUNT, 0));
    if (index < 0 || index >= count) { prefCosmetics.end(); return cosmetic; }
    char key[32];
    snprintf(key, sizeof(key), "%s%d", KEY_COSMETIC_PREFIX, index);
    const String jsonString = prefCosmetics.getString(key, "");
    prefCosmetics.end();
    if (jsonString.length() == 0 || jsonString.length() >= 256) return cosmetic;
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, jsonString)) return cosmetic;
    if (!doc["uid"].is<const char*>() || !doc["name"].is<const char*>()) return cosmetic;
    cosmetic.uid = doc["uid"].as<String>();
    cosmetic.name = doc["name"].as<String>();
    cosmetic.brand = doc["brand"].is<const char*>() ? doc["brand"].as<String>() : "";
    cosmetic.expireDate = doc["expireDate"].is<const char*>() ? doc["expireDate"].as<String>() : "";
    cosmetic.registeredAt = doc["registeredAt"].is<unsigned long>() ? doc["registeredAt"].as<unsigned long>() : 0;
    if (cosmetic.uid.length() == 0 || cosmetic.name.length() == 0) return Cosmetic{"", "", "", "", 0};
    return cosmetic;
}

int getCosmeticCount() {
    prefCosmetics.begin(NAMESPACE_COSMETICS, false);
    const int count = boundedCount(prefCosmetics.getInt(KEY_COSMETIC_COUNT, 0));
    prefCosmetics.end();
    return count;
}

Cosmetic getCosmeticByUid(const String& uid) {
    const int count = getCosmeticCount();
    for (int i = 0; i < count; ++i) {
        Cosmetic cosmetic = loadCosmetic(i);
        if (cosmetic.uid == uid) return cosmetic;
    }
    return Cosmetic{"", "", "", "", 0};
}

bool deleteCosmetic(int index) {
    prefCosmetics.begin(NAMESPACE_COSMETICS, false);
    const int count = boundedCount(prefCosmetics.getInt(KEY_COSMETIC_COUNT, 0));
    if (index < 0 || index >= count) { prefCosmetics.end(); return false; }
    for (int i = index; i < count - 1; ++i) {
        char currentKey[32], nextKey[32];
        snprintf(currentKey, sizeof(currentKey), "%s%d", KEY_COSMETIC_PREFIX, i);
        snprintf(nextKey, sizeof(nextKey), "%s%d", KEY_COSMETIC_PREFIX, i + 1);
        const String data = prefCosmetics.getString(nextKey, "");
        if (data.length() == 0 || prefCosmetics.putString(currentKey, data) == 0) { prefCosmetics.end(); return false; }
    }
    char lastKey[32];
    snprintf(lastKey, sizeof(lastKey), "%s%d", KEY_COSMETIC_PREFIX, count - 1);
    const bool removed = prefCosmetics.remove(lastKey);
    const bool updated = removed && prefCosmetics.putInt(KEY_COSMETIC_COUNT, count - 1) > 0;
    prefCosmetics.end();
    return updated;
}

void clearCosmetics() {
    const int count = getCosmeticCount();
    for (int i = count - 1; i >= 0; --i) deleteCosmetic(i);
}
