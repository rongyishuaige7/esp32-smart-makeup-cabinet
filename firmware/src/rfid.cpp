#include "rfid.h"
#include "config.h"
#include "display.h"
#include <SPI.h>
#include <MFRC522.h>
#include <time.h>

static MFRC522 rfid(PIN_RC522_SS, PIN_RC522_RST);
static Cosmetic cachedCosmetics[MAX_COSMETIC_RECORDS];
static int cachedCount = 0;
static bool cacheValid = false;
static bool scanMode = false;
static bool scannedCardCaptured = false;

static bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static bool parseCalendarDate(const String& value, int& year, int& month, int& day) {
    if (value.length() != 10 || value[4] != '-' || value[7] != '-') return false;
    for (size_t i = 0; i < value.length(); ++i) {
        if (i == 4 || i == 7) continue;
        if (value[i] < '0' || value[i] > '9') return false;
    }
    year = value.substring(0, 4).toInt();
    month = value.substring(5, 7).toInt();
    day = value.substring(8, 10).toInt();
    if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1) return false;
    static const int monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maximum = monthDays[month - 1];
    if (month == 2 && isLeapYear(year)) maximum = 29;
    return day <= maximum;
}

static void reloadCache() {
    cachedCount = getCosmeticCount();
    if (cachedCount < 0) cachedCount = 0;
    if (cachedCount > MAX_COSMETIC_RECORDS) cachedCount = MAX_COSMETIC_RECORDS;
    for (int i = 0; i < cachedCount; ++i) cachedCosmetics[i] = loadCosmetic(i);
    cacheValid = true;
}

void initRFID() {
    // UID is cloneable and is not an identity, access or latch credential.
    SPI.begin(PIN_RC522_SCK, PIN_RC522_MISO, PIN_RC522_MOSI, PIN_RC522_SS);
    rfid.PCD_Init();
    reloadCache();
    Serial.println("RFID initialized; public build does not use cards as access control");
}

bool isCardPresent() { return rfid.PICC_IsNewCardPresent(); }

String readCardUID() {
    String uid;
    if (!rfid.PICC_ReadCardSerial()) return uid;
    for (byte i = 0; i < rfid.uid.size; ++i) {
        if (uid.length() > 0) uid += ":";
        if (rfid.uid.uidByte[i] < 0x10) uid += "0";
        uid += String(rfid.uid.uidByte[i], HEX);
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return uid;
}

bool registerCosmetic(const String& uid, const String& name, const String& brand, const String& expireDate) {
    if (getCosmetic(uid) != nullptr) return false;
    Cosmetic cosmetic{uid, name, brand, expireDate, millis()};
    if (!saveCosmetic(cosmetic)) return false;
    reloadCache();
    return true;
}

Cosmetic* getCosmetic(const String& uid) {
    if (!cacheValid) reloadCache();
    for (int i = 0; i < cachedCount; ++i) {
        if (cachedCosmetics[i].uid == uid) return &cachedCosmetics[i];
    }
    return nullptr;
}

ExpiryState getExpiryState(const String& uid) {
    Cosmetic* cosmetic = getCosmetic(uid);
    if (!cosmetic) return ExpiryState::InvalidRecord;
    if (cosmetic->expireDate.length() == 0) return ExpiryState::NotSet;
    int year, month, day;
    if (!parseCalendarDate(cosmetic->expireDate, year, month, day)) return ExpiryState::InvalidRecord;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return ExpiryState::TimeUnavailable;
    const int currentDate = (timeinfo.tm_year + 1900) * 10000 + (timeinfo.tm_mon + 1) * 100 + timeinfo.tm_mday;
    const int expiryDate = year * 10000 + month * 100 + day;
    return currentDate > expiryDate ? ExpiryState::Expired : ExpiryState::Valid;
}

bool isExpired(const String& uid) { return getExpiryState(uid) == ExpiryState::Expired; }

bool deleteCosmetic(const String& uid) {
    if (!cacheValid) reloadCache();
    int index = -1;
    for (int i = 0; i < cachedCount; ++i) {
        if (cachedCosmetics[i].uid == uid) { index = i; break; }
    }
    if (index < 0 || !::deleteCosmetic(index)) return false;
    reloadCache();
    return true;
}

void startRFIDScan() {
#if EXPERIMENTAL_RFID_MAINTENANCE_ENABLED
    scanMode = true;
    scannedCardCaptured = false;
#else
    scanMode = false;
    scannedCardCaptured = false;
#endif
}

void stopRFIDScan() {
    scanMode = false;
    scannedCardCaptured = false;
}

bool isRFIDScanMode() { return scanMode; }
bool hasScannedCard() { return scannedCardCaptured; }

void handleRFIDScan() {
#if EXPERIMENTAL_RFID_MAINTENANCE_ENABLED
    if (!scanMode || !rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;
    // Do not retain or log the UID in the public source path. A local operator
    // must perform any identifier entry in their own private maintenance flow.
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    scannedCardCaptured = true;
#else
    // Public default is deliberately inert.
#endif
}
