#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

// Cosmetic结构定义在此，避免与RFID模块循环依赖
struct Cosmetic {
    String uid;
    String name;
    String brand;
    String expireDate;
    unsigned long registeredAt;
};

struct SystemSettings {
    int tempThresholdHigh;
    int tempThresholdLow;
    int humidThresholdHigh;
    int humidThresholdLow;
    int lightTimeout;
};

void initStorage();
void saveSettings(SystemSettings settings);
SystemSettings loadSettings();
bool saveCosmetic(Cosmetic cosmetic);
Cosmetic loadCosmetic(int index);
int getCosmeticCount();
bool deleteCosmetic(int index);
Cosmetic getCosmeticByUid(const String& uid);
void clearCosmetics();

#endif
