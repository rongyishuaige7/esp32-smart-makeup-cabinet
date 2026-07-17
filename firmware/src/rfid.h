#ifndef RFID_H
#define RFID_H

#include <Arduino.h>
#include "storage.h"

enum class ExpiryState : uint8_t { NotSet, Valid, Expired, TimeUnavailable, InvalidRecord };

void initRFID();
bool isCardPresent();
String readCardUID();
bool registerCosmetic(const String& uid, const String& name, const String& brand, const String& expireDate);
Cosmetic* getCosmetic(const String& uid);
int getCosmeticCount();
ExpiryState getExpiryState(const String& uid);
bool isExpired(const String& uid);
bool deleteCosmetic(const String& uid);

// Scan and raw UID handling are deliberately local-experimental only. Public
// HTTP never returns a UID.
void startRFIDScan();
void stopRFIDScan();
bool isRFIDScanMode();
bool hasScannedCard();
void handleRFIDScan();

#endif
